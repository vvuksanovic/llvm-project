#include "llvm/Transforms/Scalar/FormatStringNull.h"

#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"

using namespace llvm;

FormatStringNullPass::FormatStringNullPass(){};

PreservedAnalyses FormatStringNullPass::run(Function &F,
                                            FunctionAnalysisManager &AM) {
  auto &TLI = AM.getResult<TargetLibraryAnalysis>(F);

  for (auto &BB : F) {
    for (auto &Inst : BB) {
      if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
        bool IsBounded;
        unsigned FormatStringIdx;

        LibFunc CallFunc;
        if (TLI.getLibFunc(*CI->getCalledFunction(), CallFunc)) {
          switch (CallFunc) {
          case LibFunc_printf:
          case LibFunc_small_printf:
            IsBounded = false;
            FormatStringIdx = 0;
            break;
          case LibFunc_fprintf:
          case LibFunc_small_fprintf:
          case LibFunc_sprintf:
          case LibFunc_sprintf_chk:
          case LibFunc_small_sprintf:
          case LibFunc_vsprintf:
            IsBounded = false;
            FormatStringIdx = 1;
            break;
          case LibFunc_snprintf:
          case LibFunc_snprintf_chk:
          case LibFunc_vsnprintf:
            IsBounded = true;
            FormatStringIdx = 2;
            break;
          default:
            // Not a printf function.
            continue;
          }
        } else if (CI->getCalledFunction()->hasFnAttribute(
                       Attribute::FormatPrintf)) {
          auto Attr =
              CI->getCalledFunction()->getFnAttribute(Attribute::FormatPrintf);
          IsBounded = false;
          FormatStringIdx = Attr.getValueAsInt();
        } else {
          // Not a printf-like function.
          continue;
        }

        // assert(FormatStringIdx < CI->getNumOperands());
        if (FormatStringIdx >= CI->getNumOperands())
          continue;

        // Process the format string.
        Value *FormatStrValue = CI->getArgOperand(FormatStringIdx);

        // Diagnose and exit if the format string is null.
        if (isa<ConstantPointerNull>(FormatStrValue)) {
          F.getContext().diagnose(
              DiagnosticInfoFormatStringNull(F, CI->getDebugLoc(), IsBounded));
        }
      }
    }
  }

  return PreservedAnalyses::all();
}

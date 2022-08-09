#include "llvm/Transforms/Utils/DebugInfoAnalysis.h"

#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils.h"

using namespace llvm;

static void runDebugInfoAnalysisPass(Function &F) {
  unsigned NumValue = 0;
  unsigned NumDeclare = 0;

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      /*
      // Impl 1
      // Intrinsics support isa, cast and dyn_cast
      if (dyn_cast<DbgValueInst>(&I)) {
              ++NumValue;
      } else if (dyn_cast<DbgDeclareInst>(&I)) {
              ++NumDeclare;
      }
      */

      // Impl 2
      // Cast to IntrinsicInst and get ID
      // Enum of IDs can be found in build/include/llvm/IR/IntrinsicEnums.inc
      // the actual enum is created in llvm/include/llvm/IR/Intrinsics.h
      auto *II = dyn_cast<IntrinsicInst>(&I);
      if (!II)
        continue;

      if (II->getIntrinsicID() == Intrinsic::dbg_value)
        ++NumValue;
      else if (II->getIntrinsicID() == Intrinsic::dbg_declare)
        ++NumDeclare;
    }
  }

  outs() << "Function " << F.getName() << ":\n";
  outs() << "Number of llvm.dbg.value instructions:   " << NumValue << "\n";
  outs() << "Number of llvm.dbg.declare instructions: " << NumDeclare << "\n";
}

PreservedAnalyses DebugInfoAnalysisPass::run(Function &F,
                                             FunctionAnalysisManager &AM) {
  runDebugInfoAnalysisPass(F);
  return PreservedAnalyses::all();
}

namespace {

struct DebugInfoAnalysisLegacyPass : public FunctionPass {
  static char ID;

  DebugInfoAnalysisLegacyPass() : FunctionPass(ID) {
    initializeDebugInfoAnalysisLegacyPassPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override {
    if (skipFunction(F))
      return false;

    runDebugInfoAnalysisPass(F);
    return false;
  }
};

} // end anonymous namespace

char DebugInfoAnalysisLegacyPass::ID = 0;

INITIALIZE_PASS_BEGIN(DebugInfoAnalysisLegacyPass, "debug-info-analysis",
                      "Debug Info Analysis", false, false)
INITIALIZE_PASS_END(DebugInfoAnalysisLegacyPass, "debug-info-analysis",
                    "Debug Info Analysis", false, false)

FunctionPass *llvm::createDebugInfoAnalysisPass() {
  return new DebugInfoAnalysisLegacyPass();
}

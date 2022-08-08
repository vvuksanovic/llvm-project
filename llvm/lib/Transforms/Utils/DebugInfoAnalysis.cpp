#include "llvm/Transforms/Utils/DebugInfoAnalysis.h"

#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

PreservedAnalyses DebugInfoAnalysisPass::run(Function &F,
                                             FunctionAnalysisManager &AM) {
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

  outs() << "Number of llvm.dbg.value instructions:   " << NumValue << "\n";
  outs() << "Number of llvm.dbg.declare instructions: " << NumDeclare << "\n";
  return PreservedAnalyses::all();
}

#ifndef LLVM_TRANSFORMS_SCALAR_FORMAT_STRING_BOUNDS_H
#define LLVM_TRANSFORMS_SCALAR_FORMAT_STRING_BOUNDS_H

#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm {

// class FormatStringBoundsPass : public FunctionPass {
// public:
//   static char ID;

//   FormatStringBoundsPass();

//   bool runOnFunction(Function &F) override;
// };

class FormatStringBoundsPass : public PassInfoMixin<FormatStringBoundsPass> {
public:
  FormatStringBoundsPass();

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_SCALAR_FORMAT_STRING_BOUNDS_H
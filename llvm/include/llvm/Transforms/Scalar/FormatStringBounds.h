#ifndef LLVM_TRANSFORMS_SCALAR_FORMAT_STRING_BOUNDS_H
#define LLVM_TRANSFORMS_SCALAR_FORMAT_STRING_BOUNDS_H

#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm {

class FormatStringBoundsPass : public PassInfoMixin<FormatStringBoundsPass> {
public:
  FormatStringBoundsPass(int Level = 1);

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

private:
  int Level;
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_SCALAR_FORMAT_STRING_BOUNDS_H

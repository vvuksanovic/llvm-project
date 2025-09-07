#ifndef LLVM_TRANSFORMS_SCALAR_FORMAT_STRING_NULL_H
#define LLVM_TRANSFORMS_SCALAR_FORMAT_STRING_NULL_H

#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm {

class FormatStringNullPass : public PassInfoMixin<FormatStringNullPass> {
public:
  FormatStringNullPass();

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_SCALAR_FORMAT_STRING_NULL_H

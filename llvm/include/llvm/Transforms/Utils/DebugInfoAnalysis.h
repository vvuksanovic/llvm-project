#ifndef LLVM_TRANSFORMS_UTILS_DEBUGINFOANALYSIS_H
#define LLVM_TRANSFORMS_UTILS_DEBUGINFOANALYSIS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class DebugInfoAnalysisPass : public PassInfoMixin<DebugInfoAnalysisPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_DEBUGINFOANALYSIS_H

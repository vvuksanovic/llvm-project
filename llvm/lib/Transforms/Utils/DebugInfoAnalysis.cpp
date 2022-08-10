#include "llvm/Transforms/Utils/DebugInfoAnalysis.h"

#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils.h"

using namespace llvm;

static bool runDebugInfoAnalysisPass(Function &F) {
  unsigned NumValue = 0;
  unsigned NumDeclare = 0;

  DIBuilder DIB(*F.getParent(), false);

  for (BasicBlock &BB : F) {
    for (Instruction &I : make_early_inc_range(BB)) {
      if (auto *DVI = dyn_cast<DbgValueInst>(&I)) {
        ++NumValue;
        DIB.insertDeclare(DVI->getValue(0), DVI->getVariable(),
                          DVI->getExpression(), DVI->getDebugLoc(), &I);
        DVI->eraseFromParent();
      } else if (auto *DDI = dyn_cast<DbgDeclareInst>(&I)) {
        ++NumDeclare;
        DIB.insertDbgValueIntrinsic(DDI->getAddress(), DDI->getVariable(),
                                    DDI->getExpression(), DDI->getDebugLoc(),
                                    &I);
        DDI->eraseFromParent();
      }
    }
  }

  outs() << "Function " << F.getName() << ":\n";
  outs() << "Number of llvm.dbg.value instructions:   " << NumValue << "\n";
  outs() << "Number of llvm.dbg.declare instructions: " << NumDeclare << "\n";

  return NumValue != 0 || NumDeclare != 0;
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

    return runDebugInfoAnalysisPass(F);
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

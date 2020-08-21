#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static char PASS_NAME[] = "CountStaticInstrPass";
static char PASS_VERSION[] = "v0.1";
static char ARGUMENT_NAME[] = "csi";

namespace {
struct CountStaticInstrPass : public PassInfoMixin<CountStaticInstrPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
      for (auto &BB: F) {
          for (auto &Instr: BB) {
              errs() << Instr.getOpcodeName();
          }
      }

    return PreservedAnalyses::all();
  }
};
}

bool CallBackFunction(StringRef Name, FunctionPassManager &FPM,
                      ArrayRef<PassBuilder::PipelineElement>) {
  if (Name == ARGUMENT_NAME) {
    FPM.addPass(CountStaticInstrPass());
    return true;
  } else {
    return false;
  }
}

void RegistrationFunction(PassBuilder &PB) {
  PB.registerPipelineParsingCallback(CallBackFunction);
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {

  return {LLVM_PLUGIN_API_VERSION, PASS_NAME, PASS_VERSION,
          RegistrationFunction};
}
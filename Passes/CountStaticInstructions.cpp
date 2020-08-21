#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <map>

using namespace llvm;

static char PASS_NAME[] = "CountStaticInstrPass";
static char PASS_VERSION[] = "v0.1";
static char ARGUMENT_NAME[] = "csi";

namespace {
struct CountStaticInstrPass : public PassInfoMixin<CountStaticInstrPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {

    std::map<const char *, int> InstrCount;

    for (auto &BB : F) {
      for (auto &Instr : BB) {
        auto InstrName = Instr.getOpcodeName();
        if (InstrCount.find(InstrName) == InstrCount.end()) {
          // If intruction is not previously seen, add it to InstrCount map
          InstrCount.insert({InstrName, 1});
        } else {
          // Otherwise increment count
          InstrCount[InstrName] += 1;
        }
      }
    }

    // Iterate over map to print all instructions
    for (auto &i : InstrCount) {
      errs() << i.first << "\t" << i.second << "\n";
    }

    return PreservedAnalyses::all();
  }
};
} // namespace

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
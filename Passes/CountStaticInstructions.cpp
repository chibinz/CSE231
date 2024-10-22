#include <map>

#include "HelperFunctions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static auto PASS_NAME = "CountStaticInstrPass";
static auto PASS_VERSION = "v0.1";
static auto ARGUMENT_NAME = "csi";

namespace {
struct CountStaticInstrPass : public PassInfoMixin<CountStaticInstrPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    // Map for storing instruction count
    std::map<const char *, int> InstrCount;

    // Iterate over all basic blocks in function
    for (auto &BB : F) {
      // Iterate over all instructions in basic block
      for (auto &I : BB) {
        // Note that `getOpcodeName` returns const char *, a fixed pointer.
        // As long as the instruction have the same opcode name, the returned
        // pointer will be the same.
        // That's why you can use it as the key for the map.
        auto InstrName = I.getOpcodeName();

        // If intruction is not previously seen, add it to InstrCount map
        // Otherwise increment count
        mapInsertOrIncrement(InstrCount, InstrName, 1);
      }
    }

    // Iterate over map to print out instruction counts
    for (auto &i : InstrCount) {
      errs() << i.first << "\t" << i.second << "\n";
    }

    // Nothing is changed, all previous analyses are preserved
    return PreservedAnalyses::all();
  }
};
} // namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, PASS_NAME, PASS_VERSION,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == ARGUMENT_NAME) {
                    FPM.addPass(CountStaticInstrPass());
                    return true;
                  } else {
                    return false;
                  }
                });
          }};
}
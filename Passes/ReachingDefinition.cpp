#include <set>

#include "Bimap.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Type.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static auto PASS_NAME = "ReachingDefinitionPass";
static auto PASS_VERSION = "v0.1";
static auto ARGUMENT_NAME = "reaching";

// Stuffs to implement
// Transfer functions - operate on instructions
// Join operator - operate on basic blocks
// Instruction indexing

Bimap<Value *, unsigned> InstrIndexBimap;
using GenSet = std::set<unsigned>;
using KillSet = std::set<unsigned>;

auto indexInstructions(Function &F) {
  auto count = 0;

  for (auto &BB: F) {
    for (auto &I: BB) {
      count += 1;
      InstrIndexBimap.insert(&I, count);
    }
  }
}

namespace {
struct ReachingDefinitionPass : public PassInfoMixin<ReachingDefinitionPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {

    return PreservedAnalyses::none();
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
                    FPM.addPass(ReachingDefinitionPass());
                    return true;
                  } else {
                    return false;
                  }
                });
          }};
}
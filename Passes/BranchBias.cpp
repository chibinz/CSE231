#include <map>

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static auto PASS_NAME = "ProfileBranchBiasPass";
static auto PASS_VERSION = "v0.1";
static auto ARGUMENT_NAME = "bb";

namespace {
struct ProfileBranchBiasPass : public PassInfoMixin<ProfileBranchBiasPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {

    for (auto &BB : F) {
    }

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
                    FPM.addPass(ProfileBranchBiasPass());
                    return true;
                  } else {
                    return false;
                  }
                });
          }};
}
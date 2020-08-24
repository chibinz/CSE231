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
    auto M = F.getParent();
    auto &CTX = M->getContext();

    auto updateType =
        FunctionType::get(Type::getVoidTy(CTX), {Type::getInt1Ty(CTX)}, false);
    auto updateFunc = M->getOrInsertFunction("__updateBrCount__", updateType);

    for (auto &BB : F) {
      // No need to iterate over every instruction.
      // A basic block is by-definition terminated by a
      // branch / jump / call / return instruction
      auto terminator = BB.getTerminator();

      // Filter for branch instructions
      if (terminator->getOpcode() == Instruction::Br) {
        auto BrInstr = dyn_cast<BranchInst>(terminator);

        // Filter for conditional branches
        if (BrInstr != nullptr && BrInstr->isConditional()) {
          // Extract condition from branch instruction and
          // assert it is a boolean
          auto Cond = BrInstr->getCondition();
          assert(Cond->getType() == Type::getInt1Ty(CTX));

          // Insert call to update `BrCount` pair
          CallInst::Create(updateFunc, {Cond}, "", terminator);
        }
      }
    }

    // Construct function signature and insert call to
    // print out count of branches taken
    auto printType = FunctionType::get(Type::getVoidTy(CTX), false);
    auto printFunc =
        M->getOrInsertFunction("__printAndClearBrCount__", printType);
    auto retInstr = F.back().getTerminator();
    CallInst::Create(printFunc, "", retInstr);

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
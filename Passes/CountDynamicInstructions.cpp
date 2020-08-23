#include <map>

#include "HelperFunctions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Type.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static auto PASS_NAME = "CountDynamicInstrPass";
static auto PASS_VERSION = "v0.1";
static auto ARGUMENT_NAME = "cdi";

namespace {
struct CountDynamicInstrPass : public PassInfoMixin<CountDynamicInstrPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    // Get context to construct function type
    auto M = F.getParent();
    auto &CTX = M->getContext();

    auto i32Ty = IntegerType::getInt32Ty(CTX);
    auto updateType =
        FunctionType::get(IntegerType::getVoidTy(CTX), {i32Ty, i32Ty}, false);
    FunctionCallee updateFunc =
        M->getOrInsertFunction("__updateInstrCount__", updateType);

    std::map<unsigned, int> instrCount;
    Instruction *terminator;

    for (auto &BB : F) {
      for (auto &Instr : BB) {
        mapInsertOrIncrement(instrCount, Instr.getOpcode(), 1);
        terminator = BB.getTerminator();
      }

      for (auto &pair : instrCount) {
        Value *key = ConstantInt::get(i32Ty, pair.first);
        Value *value = ConstantInt::get(i32Ty, pair.second);
        CallInst::Create(updateFunc, {key, value}, "", terminator);
      }

      instrCount.clear();
    }

    if (F.getName() == "main") {
      auto printType = FunctionType::get(IntegerType::getVoidTy(CTX), false);
      auto printFunc = M->getOrInsertFunction("__printInstrCount__", printType);
      // Last instruction of main function
      auto lastInstr = F.back().getTerminator();
      CallInst::Create(printFunc, "", lastInstr);
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
                    FPM.addPass(CountDynamicInstrPass());
                    return true;
                  } else {
                    return false;
                  }
                });
          }};
}
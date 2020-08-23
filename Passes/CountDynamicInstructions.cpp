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
    // Modules, Functions, BasicBlocks are often compacted by their initials
    auto M = F.getParent();
    // Context is necessary for getting types
    auto &CTX = M->getContext();

    auto i32Ty = IntegerType::getInt32Ty(CTX);
    auto updateType = // (return type, {parameter type, ...}, is_variadic)
        FunctionType::get(IntegerType::getVoidTy(CTX), {i32Ty, i32Ty}, false);
    auto updateFunc = // (symbol name, function type)
        M->getOrInsertFunction("__updateInstrCount__", updateType);

    // Temporary map for storing instruction counts in each basic block
    // Modified at compile time
    std::map<unsigned, int> instrCount;

    for (auto &BB : F) {
      for (auto &Instr : BB) {
        mapInsertOrIncrement(instrCount, Instr.getOpcode(), 1);
      }

      // For each entry (opcode, count) in the temporary map,
      // insert a call to `__updateInstrCount__` before any `br`
      // instruction, i.e exiting the basic block.
      for (auto &pair : instrCount) {
        auto terminator = BB.getTerminator();
        auto key = ConstantInt::get(i32Ty, pair.first);
        auto value = ConstantInt::get(i32Ty, pair.second);
        // Insert `updateFunc` with argument (key, value) before `terminator`
        CallInst::Create(updateFunc, {key, value}, "", terminator);
      }

      instrCount.clear();
    }

    // Insert a call to `__printInstrCount__` to print out dynamic instruction
    // counts before main function returns
    if (F.getName() == "main") {
      auto printType = FunctionType::get(IntegerType::getVoidTy(CTX), false);
      auto printFunc = M->getOrInsertFunction("__printInstrCount__", printType);
      auto mainRet = F.back().getTerminator();
      CallInst::Create(printFunc, "", mainRet);
    }

    // Since we inserted some instructions, conservatively tell `opt`
    // that nothing is preserved.
    // If you know what you're doing, a better idea would be:
    // `auto analyses = PreservedAnalyses::all();`
    // `analyses.abandon(...); ...`
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
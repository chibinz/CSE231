#include <map>
#include <set>

#include "Bimap.h"
#include "HelperFunctions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static auto PASS_NAME = "ReachingDefinitionAdhocPass";
static auto PASS_VERSION = "v0.1";
static auto ARGUMENT_NAME = "reaching";

namespace {
struct ReachingDefinitionAdhocPass
    : public PassInfoMixin<ReachingDefinitionAdhocPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    auto instrIndexBimap = indexInstrs(F);

    // Definitions going out from an instruction
    std::map<Instruction *, std::set<Instruction *>> out;

    // Definitions flowing into an instruction
    std::map<Instruction *, std::set<Instruction *>> in;

    // Set of instructions whose predecessors's `out` set (its own `in` set)
    // changed in the last iterations
    std::set<Instruction *> changed;

    // Initialize
    for (auto &BB : F) {
      for (auto &I : BB) {
        out[&I] = std::set<Instruction *>();
        in[&I] = std::set<Instruction *>() changed.insert(&I);
      }
    }

    while (!changed.empty()) {
      auto node = set::pop(changed);

      for (auto &pred : getInstrPred(*node)) {
        // `merge` method of `std::set` actually clears the second set!
        // took me hours of debugging!
        // auto copy = out[pred];
        // in[node].merge(copy);
        in[node] = set::union2(in[node], out[pred]);
      }

      // Under the ssa assumption:
      // Only one definition per instruction, gen[node] is the return value of
      // node No definition is killed, SSA implies no redefinition of variables

      auto old_out = out[node];
      out[node] = in[node];

      // `Br`, `Ret`, and `Store`, etc. do not have a return value, and thus
      // does not generate a definition
      if (!noRetValue(*node)) {
        out[node].insert(node);
      }

      // If `out[node]` is updated, add all successors of `node` to `changed`
      if (old_out != out[node]) {
        for (auto &succ : getInstrSucc(*node)) {
          changed.insert(succ);
        }
      }
    }

    // Print out reaching definitions for each instruction
    for (auto &[instr, defs] : out) {
      errs() << instrIndexBimap.find(instr) << ":\t";
      instr->print(errs());
      errs() << ":\t{";
      for (auto &def : defs) {
        errs() << instrIndexBimap.find(def) << " ";
      }
      errs() << "}\n";
    }

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
                    FPM.addPass(ReachingDefinitionAdhocPass());
                    return true;
                  } else {
                    return false;
                  }
                });
          }};
}
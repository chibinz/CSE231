#include <map>
#include <set>

#include "Bimap.h"
#include "HelperFunctions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Type.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static auto PASS_NAME = "ReachingDefinitionPass";
static auto PASS_VERSION = "v0.1";
static auto ARGUMENT_NAME = "reaching";

/// Return set of predecessor(s) of a given instruction
static auto getInstrPred(Instruction &I) -> std::set<Instruction *> {
  std::set<Instruction *> result = {};
  auto prev = I.getPrevNode();
  auto parentBlock = I.getParent();

  // Predecessors of an instruction come in 3 cases
  // 1. First instruction in function => {}
  // 2. First instruction in basic block but not first in function
  //    => {Terminators of preceding basic block(s)}
  // 3. Following instructions in a basic block => {Previous instruction}

  if (prev == nullptr) {
    for (auto pred : predecessors(parentBlock)) {
      result.insert(pred->getTerminator());
    }
  } else {
    result.insert(prev);
  }

  return result;
}

/// Return set of successors of a given instruction
static auto getInstrSucc(Instruction &I) -> std::set<Instruction *> {
  std::set<Instruction *> result = {};
  auto next = I.getNextNode();

  if (next == nullptr) {
    for (auto succ : successors(I.getParent())) {
      result.insert(&succ->front());
    }
  } else {
    result.insert(next);
  }

  return result;
}

namespace {
struct ReachingDefinitionPass : public PassInfoMixin<ReachingDefinitionPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    auto instrIndexBimap = indexInstructions(F);

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
        in[&I] = std::set<Instruction *>();
        changed.insert(&I);
      }
    }

    while (!changed.empty()) {
      auto node = pop(changed);

      for (auto &pred : getInstrPred(*node)) {
        // `merge` method of `std::set` actually clears the second set!
        // took me hours of debugging!
        auto copy = out[pred];
        in[node].merge(copy);
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
                    FPM.addPass(ReachingDefinitionPass());
                    return true;
                  } else {
                    return false;
                  }
                });
          }};
}
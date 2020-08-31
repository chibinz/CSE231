#include <map>
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

Bimap<Instruction *, unsigned> InstrIndexBimap;

auto noRetValue(Instruction &I) -> bool {
  auto opcode = I.getOpcode();

  return opcode == Instruction::Br || opcode == Instruction::Ret ||
         opcode == Instruction::Switch || opcode == Instruction::Store;
}

auto indexInstructions(Function &F) {
  auto count = 0;

  for (auto &BB : F) {
    for (auto &I : BB) {
      count += 1;
      InstrIndexBimap.insert(&I, count);
    }
  }
}

/// Return set of predecessor(s) of a given instruction
auto getInstrPred(Instruction &I) -> std::set<Instruction *> {
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

auto getInstrSucc(Instruction &I) -> std::set<Instruction *> {
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
    indexInstructions(F);

    std::map<Instruction *, std::set<Instruction *>> out;
    std::map<Instruction *, std::set<Instruction *>> in;

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
      auto node = *changed.begin();
      changed.erase(changed.begin());

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

    for (auto &[instr, defs] : out) {
      errs() << InstrIndexBimap.find(instr) << ":\t";
      instr->print(errs());
      errs() << ":\t{";
      for (auto &def : defs) {
        errs() << InstrIndexBimap.find(def) << " ";
      }
      errs() << "}\n";
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
                    FPM.addPass(ReachingDefinitionPass());
                    return true;
                  } else {
                    return false;
                  }
                });
          }};
}
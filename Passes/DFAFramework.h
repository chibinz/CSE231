#pragma once

#include <map>
#include <string>

#include "HelperFunctions.h"
#include "llvm/IR/Instruction.h"

using namespace llvm;

/// Generic class for representing lattice values / nodes
/// The meet operator is used for composing several lattice values
/// and the equal operator is used to check if a lattice has changed
class Info {
public:
  Info(const Info &other){};
  /// Convenience function for test and debug printing
  virtual auto dump() -> std::string;
  /// Check if 2 lattice values are equivalent to each other
  static auto equal(const Info &a, const Info &b) -> bool;
  /// Compose 2 lattice values into a new one
  /// Should be 1. idempotent 2. commutative 3. associative
  static auto meet(const Info &a, const Info &b) -> Info;
};

template <class Info, bool Direction>
class DataFlowAnalysis {
public:
  DataFlowAnalysis(Info lattice_top, Function &F) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        in[&I] = top;
        out[&I] = top;
        worklist.insert(&I);
      }
    }
  };

  auto run() -> void {
    while (!worklist.empty()) {
      // Select and remove a node (instruction) from the work list
      auto node = set::pop(worklist);

      // Apply `meet` operators to output values of all predecessors
      for (auto &pred : getInstrPred(*node)) {
        in[node] = Info::meet(in[node], out[pred]);
      }

      auto old_out = out[node];
      // Apply transfer function
      out[node] = transferFunction(node, in[node]);

      // Add to worklist if output changed
      if (!Info::equal(out[node], old_out)) {
        for (auto &succ : getInstrSucc(*node)) {
          worklist.insert(succ);
        }
      }
    }
  };

  static auto transferFunction(const Instruction *instr, const Info &input)
      -> Info;

private:
  std::map<Instruction *, Info> in;
  std::map<Instruction *, Info> out;
  std::set<Instruction *> worklist;
  Instruction *entry;
};
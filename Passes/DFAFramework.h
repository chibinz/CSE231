#include <map>
#include <string>

#include "HelperFunctions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

/// Generic class for representing lattice values / nodes
/// The meet operator is used for composing several lattice values
/// and the equal operator is used to check if a lattice has changed
// class Info {
// public:
//   virtual ~Info() {}
//   /// Convenience function for test and debug printing
//   virtual auto dump() -> std::string = 0;

//   /// Check if 2 lattice values are equivalent to each other
//   virtual auto operator==(const Info &other) const -> bool = 0;

//   /// Compose 2 lattice values into a new one
//   /// Should be 1. idempotent 2. commutative 3. associative
//   virtual auto operator^(const Info &other) const -> Info * = 0;
// };

template <class Info, bool Direction> class DataFlowAnalysis {
public:
  DataFlowAnalysis(Info lattice_top, Function &F) : func(F) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        in[&I] = lattice_top;
        out[&I] = lattice_top;
        worklist.insert(&I);
      }
    }
  }

  virtual ~DataFlowAnalysis() {}

  auto run() -> void {
    while (!worklist.empty()) {
      // Select and remove a node (instruction) from the work list
      auto node = set::pop(worklist);

      // Apply `meet` operators to output values of all predecessors
      for (auto &pred : getInstrPred(*node)) {
        in[node] = in[node] ^ out[pred];
      }

      auto old_out = out[node];
      // Apply transfer function
      out[node] = transferFunction(node, in[node]);

      // Add to worklist if output changed
      if (!(out[node] == old_out)) {
        for (auto &succ : getInstrSucc(*node)) {
          worklist.insert(succ);
        }
      }
    }
  }

  virtual auto print() -> void {
    auto map = indexInstrs(func);
    for (auto &BB : func) {
      for (auto &I : BB) {
        errs() << map.find(&I) << "\t:";
        I.print(errs());
        errs() << "\n"
               << "in"
               << "\t: ";
        in[&I].print(map);
        errs() << "\n"
               << "out"
               << "\t: ";
        out[&I].print(map);
        errs() << "\n";
      }
    }
  }

  virtual auto transferFunction(Instruction *instr, Info input) -> Info = 0;

private:
  std::map<Instruction *, Info> in;
  std::map<Instruction *, Info> out;
  std::set<Instruction *> worklist;
  Function &func;
};
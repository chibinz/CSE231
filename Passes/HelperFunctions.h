#pragma once

#include <map>
#include <set>

#include "Bimap.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

using namespace llvm;

template <typename T>
/// Add or increment a key's corresponding value
static auto mapInsertOrIncrement(std::map<T, int> &map, T key, int count) {
  if (map.find(key) == map.end()) {
    // On first time seeing key, insert new entry
    map.insert({key, count});
  } else {
    // Otherwise add count to current value
    map[key] += count;
  }
}

namespace set {
template <typename T>
/// Remove and return an element from a set
static auto pop(std::set<T> &set) -> T {
  auto front = *set.begin();
  set.erase(set.begin());
  return front;
}

template <typename T>
/// Return union of 2 sets, append `2` to avoid conflict with keyword union
static auto union2(const std::set<T> &a, const std::set<T> &b) -> std::set<T> {
  auto copy_a = std::set<T>(a);
  auto copy_b = std::set<T>(b);
  copy_a.merge(copy_b);
  return copy_a;
}
} // namespace set

/// Assign index to each instruction
static inline auto indexInstrs(Function &F) {
  auto bimap = Bimap<Instruction *, unsigned>();
  auto count = 0;

  for (auto &BB : F) {
    for (auto &I : BB) {
      count += 1;
      bimap.insert(&I, count);
    }
  }

  return bimap;
}

/// Return true if given instruction does not have a return value,
/// i.e. opcode = `Br` / `Ret` / `Switch` / `Store`.
static inline auto noRetValue(const Instruction &I) -> bool {
  auto opcode = I.getOpcode();

  return opcode == Instruction::Br || opcode == Instruction::Ret ||
         opcode == Instruction::Switch || opcode == Instruction::Store;
}

/// Return true if the given instruction has a return value,
/// e.g. `Add`, `Mul`, `Select`, `Call`
static inline auto hasRetValue(const Instruction &I) -> bool {
  return !noRetValue(I);
}

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
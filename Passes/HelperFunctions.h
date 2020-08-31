#pragma once

#include <map>
#include <set>

#include "Bimap.h"
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

template <typename T>
/// Remove and return an element from a set
static auto pop(std::set<T> &set) {
  auto front = *set.begin();
  set.erase(set.begin());
  return front;
}

/// Assign index to each instruction
static inline auto indexInstructions(Function &F) {
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
static inline auto noRetValue(Instruction &I) -> bool {
  auto opcode = I.getOpcode();

  return opcode == Instruction::Br || opcode == Instruction::Ret ||
         opcode == Instruction::Switch || opcode == Instruction::Store;
}
#include <iostream>
#include <map>

#include "llvm/IR/Instruction.h"

// Store opcode instead of string to avoid memory allocation issues
std::map<unsigned, int> InstrCount;

extern "C"
void __updateInstrCount__(unsigned opcode, int count) {
  if (InstrCount.find(opcode) == InstrCount.end()) {
    // Add to InstrCount map if opcode not seen before
    InstrCount.insert({opcode, count});
  } else {
    // Otherwise increment count
    InstrCount[opcode] += count;
  }
}

extern "C"
void __printInstrCount__() {
  for (auto &pair: InstrCount) {
    auto name = llvm::Instruction::getOpcodeName(pair.first);
    std::cerr << name << "\t" << pair.second << "\n";
  }
}
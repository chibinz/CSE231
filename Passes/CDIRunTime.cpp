#include <iostream>
#include <map>

#include "HelperFunctions.h"
#include "llvm/IR/Instruction.h"

// Store opcode instead of string to avoid memory allocation issues.
// Modified at run time
std::map<unsigned, int> InstrCount;

// `extern "C"` is necessary for keeping the compiler from mangling
// function names. Remove this line and the linker will complain that
// there are no reference to `__updateInstrCount__`
extern "C"
auto __updateInstrCount__(unsigned opcode, int count) -> void {
    // Add to InstrCount map if opcode not seen, otherwise increment count
    mapInsertOrIncrement(InstrCount, opcode, count);
}

extern "C"
auto __printAndClearInstrCount__() {
  for (auto &[key, value]: InstrCount) {
    auto name = llvm::Instruction::getOpcodeName(key);
    std::cerr << name << "\t" << value << "\n";
  }

  InstrCount.clear();
}
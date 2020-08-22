# CSE231
Notes, project following UC San Diego's Advanced Compilers course

## Collecting Static Instruction Counts

### Running
```sh
opt -load-pass-plugin ./Build/libCountStaticInstructions.so -passes=csi ./Tests/<input>.ll -disable-output
```

### Sample Output
```text
call    2
ret     1
load    5
store   5
alloca  5
add     2
ret     1
call    1
ret     1
```

### Notes
This is just a warmup pass to get yourself familiarized with LLVM passes. LLVM made is super simple for you to iterate over functions and basic blocks by implementing iterators for class `Function` and `BasicBlock`. A typical LLVM function pass looks something like this:
```C++
// Iterate over all basic blocks in function
for (auto &BB : F) {
    // Iterate over all instructions in basic block
    for (auto &Instr : BB) {
        // Filter and manipulate instructions
    }
}
```
Directions in project description are already very clear, the following are a few key points.
- `std::map <const char*, int>` is a good candidate for storing instruction opcode names and their respective counts.
- Output to stderr using `llvm::errs() << name << "\t" << count << "\n"`
- use `-disable-output` instead of `> /dev/null`

# CSE231
Notes and projects following UC San Diego's Advanced Compilers course

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
This is just a warmup pass to get familiarized with LLVM passes. LLVM made it super simple for us to iterate over functions and basic blocks by implementing iterators for class `Function` and `BasicBlock`. A typical LLVM function pass looks something like this:
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

## Collecting Dynamic Instruction Counts
There are a few confusions that I met with through out section 2 of this project. First of all, since the input is compiled down to native code, we're actually executing x86_64 instructions instead of LLVM IR instructions. That is to say, the dynamic instruction count here is assumming that we're interpreting LLVM bitcode rather than running it. Second, the last direction states that functions instrumented will not make any calls to other functions. To have a runnable executable, there must be a main function and that main function cannot make any function calls. Why don't the direction just say that there is only one main function? This seems to conflict with "Each instrumented function should output the analysis results before function termination"...

This sections mainly aims to teach us how to insert function calls. In LLVM it usually works like this. First you need get a `Context`; `Context`, in my understanding, is an LLVM class encapsulating information about your machine. For example, type information about 32-bit integers. How these integers are represented on x86_64 may differ from, say, ARM64 machines. Once we've obtained the context, we can construct types ground up, from basic integer types to function types. `Module::getOrInsertFunction` gives us a handle to the function that we want to call, given the function name and type. Hints in the project description suggest that we use `IRBuilder` to insert the actual function call, but I find `CallInst::Create` easier to use. Just pass in the function handle, actual parameters, and the instruction that you want to insert the call before.

### Tips
- `opt ... -S` can directly output LLVM assembly, no need to use `llvm-dis`.
- Using the provided runtime library `lib231` requires you to construct arrays of key and value at compile time, which can be a bit complicated. My solution is to simply insert a function call for every opcode. Overhead is not an issue here.:)
- You can `.cpp .ll` files together, and clang will still produce an executable happily.

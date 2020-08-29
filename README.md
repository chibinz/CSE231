# CSE231
Notes and projects following UC San Diego's Advanced Compilers course

## Running
```sh
# Build all the passes
mkdir Build
meson Build
ninja -C Build

# Run a pass
opt -load-pass-plugin ./Build/libCountStaticInstructions.so -passes=csi ./Tests/<input>.ll -disable-output
```

## Collecting Static Instruction Counts

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

## Profiling Branch Bias
The main goal of section, I assume, is to teach you how to filter for a specific type of instruction, in this case the conditional branch instruction. There are probably a dozen ways to do that, I'll list 3 ways that I found comfortable to use in the following:
```C++
// Filter by checking instruction opcode
if (Instr.getOpcode() == Instruction::Br) {
    // Instr is indeed a branch instruction
}

// Filter by downcasting
BranchInst* BrInstr = dyn_cast<BranchInst>(Instr);
if (Instr != nullptr) {
    // `dyn_cast` is an LLVM helper function that attempts to cast a pointer
    // to an instance of a base class to its derived class, and returns
    // a `nullptr` if it fails to do so.
}

// Filter by instruction name
if (Instr.getOpcodeName() == "br") {
    // Manipulate `Instr`
}
```
The benefit of using `Instruction::getOpcode` is that its return type is `unsigned`. Scenario's in which you have to deal with various types of instructions can be handled efficiently with a single switch case:
```C++
switch (Instr.getOpcode()) {
    case Instruction::Alloca: ...; break;
    case Instruction::Add:    ...; break;
    case Instruction::Load:   ...; break;
    case Instruction::Br:     ...; break;
}
```
Downcasting with `dyn_cast`, on the other hand, gives us access to specific member functions of the derived class. In the conditional branch case, the `isConditional` function is only available in the `BranchInst` class, not implemented in the base `Instruction` class. A common gotcha in using `dyn_cast` is putting in the pointer type instead of the class type in the template parameter.
```C++
// This will not compile...
// The compiler will spit out screens of barely comprehensible errors.
BranchInst* BrInstr = dyn_cast<BranchInst*>(Instr);
```
The last filtering technique is quite straight forward, but is less used in practice because it offers similar functionality as the fist technique, while incurring more overhead.

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
The main goal of section, I assume, is to teach you how to filter for a specific type of instruction, in this case the conditional branch instruction. There are probably a dozen ways to do that. Listed in the following are ways that I found comfortable using:
```C++
// Handle multiple cases
switch (Instr.getOpcode()) {
    case Instruction::Br: ...; break;
    ...
}

// Filter by downcasting
BranchInst* BrInstr = dyn_cast<BranchInst>(&Instr);
if (Instr != nullptr) {
    // `dyn_cast` is an LLVM helper function that attempts to cast a pointer
    // to an instance of a base class to its derived class, and returns
    // a `nullptr` if it fails to do so.
}

// A better name would be isinstance
if (isa<BranchInst>(&Instr)) {
    // Yes, `Instr` "is a" `BranchInst`
}

// Filter by instruction name
if (Instr.getOpcodeName() == "br") {
    // Manipulate `Instr`
}
```
The benefit of using `Instruction::getOpcode` is that its return type is `unsigned`. Scenario's in which you have to deal with various types of instructions can be handled efficiently with a single switch case. If you just want to check if an object **is an instance** of a certain class, use the `isa` function. `isa` is a bad name in my opinion because I often mistake it for Instruction Set Architecture... Downcasting with `dyn_cast`, on the other hand, gives us access to specific member functions of the derived class. In the conditional branch case, the `isConditional` function is only available in the `BranchInst` class, not implemented in the base `Instruction` class. A common gotcha in using `dyn_cast` is putting in the pointer type instead of the class type in the template parameter.
```C++
// This will not compile...
// The compiler will spit out screens of barely comprehensible errors.
BranchInst* BrInstr = dyn_cast<BranchInst*>(Instr);
```
The last filtering technique is quite straight forward, but is less used in practice because it offers similar functionality as the first technique, while incurring more overhead.

## Implment Reaching Definition Analyses (Adhoc)
Unfortunately, as an outsider I do not have access to recitation and lectures. My only reference are slides from my own compiler course and the *Compilers: Principles, Techniques, and Tools* text book, and thus jumping right into the given template in `231DFA.h` is kind of overwhelming for me. So I decided to implement non-generic data flow analyses to get myself familiarized with the process. At the core of each data flow analyses is its transfer function, it determines the type of the analyses info, and how it is manipulated by each statement(instruction in implementation). In the reaching definition case, the type is set of defintions, and the transfer function states that the output is the **definition generated by this statement** plus the **definitions from the input except those killed by this instruction**. A definition, in my understanding, is just an assignment. In LLVM, most computational instructions have return values, with a few control flow instructions that don't have left handsides. Set of definitions can be represented by std::set<Instruction *>, containing instructions with a return value. The project description seems to handle `phi` instructions differently. I treat them just an any other instruction with a return value, not sure what is the problem here. LLVM IR adhering to the SSA requirement makes it super easy to compute the *gen* set and *kill* set. Since each instruction only assigns to one variable, the *gen* is simply the return value of the instruction(which turn out to be the instruction itself in LLVM, Instruction \* is a subtype of Value \*). And since no variable is assigned twice, the *kill* set is always an empty set. Note that in any cases, the *gen* and *kill* set only need to be computed once and stay fixed throughout the iterative procedure, what keeps changing is the *in* and *out* set of each statement.
There are quite a few distinction between the project requirement and the textbook. First, the *meet* operator introduced by the text book operates on basic blocks, while the output printed by `231_solution.so` is on instruction granularity. The problem with this is that llvm only supports getting successors / predecessors of basic blocks but not instructions. For terminating and leading instructions getting their respective successors and predecessors using `getNext/PrevNode` will return `nullptr`. I was able to get around this by wrapping edge cases in a function, but previously expected LLVM would offer readily available APIs... Second, transfer function in the textbook seems to correspond to flow function in the project description, which made it a little harder for me to comprehend at first sight.

### Gotchas
- When doing a union for 2 sets, be careful with `std::set::merge`. A.merge(B) does not have a return value (C = A.merge(b) will not compile), and the merge operation actually clears set B. I was trying to print out reaching definitions for each instruction at the end of the function pass, but found out that all but the terminating instructions' *out* set is empty. It took me quite a well to figure what the problem was(Rust's borrow checker won't allow such things to happen...)
- Inconsistency in getting the last and first instruction of a basic block. To get the last instruction, I used the `getTerminator` method. So I was expecting `getLeader` method of some sort to get me the first instruction...The right method to use turned out to be `front`. Since `BasicBlock` is an iterable class, it shouldn't be surprising, but what really struck me was that the `back` method was deleted. What is the rationale behind such weirdness..."
- To make a copy of custom structure in Rust, you have to explicitly the `clone` method. In C++, this seems to be done by overloading operator `=`...


## Generic Framework
###
```C++
//---- Inheriting constructors ----//
class B : public A {
    // Inherit all constructors of base class A
    using A::A;
}

//---- virtual function weirdness ----//
// Must be overriden
virtual auto func_name() = 0;

// Actually both a declaration and a definition / implementation
virtual auto func_name() {}

// What about the following?
virtual auto func_name();
virtual auto func_name() {};
```

- Unnecessary polymorphic Info type?
  Delete base `Info` class, and remove inheritance declaration(:public Info), code compiles just fine...
- Questionable use of static functions
  The point of `Info` being an abstract class is that its derived class must implement the `equal` and `meet` operator. Static function are specific to each class and cannot be virtual, i.e. derived class doesn't have to implement it. If this is the case, why not just use virtual operator overloading? The compiler does not check whether the derived class has function `equal` and `meet` function when `Info` is passed in as template parameter, but the linker will complain undefined reference...
- I previously thought that derived class must implement pure virtual functions, or else the compiler would complain...But this doesn't seem to be case, it would compile just fine

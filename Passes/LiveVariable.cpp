#include "DFAFramework.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static auto PASS_NAME = "LiveVariablePass";
static auto PASS_VERSION = "v0.1";
static auto ARGUMENT_NAME = "liveness";

class VarInfo {
public:
  /// Interestingly, while the default constructor is never explicitly called,
  /// removing it will result in a compile error
  VarInfo() {}
  VarInfo(std::set<Value *> set) : defs(set) {}

  /// Print definition set for a given statement
  /// Called by `print` method of class `DataFlowAnalysis`
  auto print(Bimap<Instruction *, unsigned> &) -> void {
    for (auto &def : defs) {
      def->printAsOperand(errs());
      errs() << " ";
    }
  }

  /// Return true if definition sets have the same definitions
  auto operator==(const VarInfo &other) const -> bool {
    return defs == other.defs;
  }

  /// Meet operator for reaching definition analysis is simply union for
  /// definition set
  auto operator^(const VarInfo &other) const -> VarInfo {
    return VarInfo(set::union2(defs, other.defs));
  }

  std::set<Value *> defs;
};

class LiveVariableAnalysis
    : public DataFlowAnalysis<VarInfo, AnalysisDirection::Backward> {
public:
  /// Inherit constructor
  using DataFlowAnalysis::DataFlowAnalysis;

  /// Note that the kill set if always empty due to the SSA property.
  /// Thus the output of the transfer function is the union of input and
  /// Definition generated by the current instruction. (if any)
  virtual auto transferFunction(Instruction *instr, VarInfo input) -> VarInfo {
    if (hasRetValue(*instr)) {
      input.defs.erase(instr);
    }
    for (auto &op : instr->operands()) {
      input.defs.insert(op);
    }
    return input;
  }
};

namespace {
struct ReachingDefinitionPass : public PassInfoMixin<ReachingDefinitionPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    // Instantiate reaching definition analysis with 'top' value of lattice
    auto analysis = LiveVariableAnalysis({}, F);

    analysis.run();
    analysis.print();

    return PreservedAnalyses::all();
  }
};
} // namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, PASS_NAME, PASS_VERSION,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == ARGUMENT_NAME) {
                    FPM.addPass(ReachingDefinitionPass());
                    return true;
                  } else {
                    return false;
                  }
                });
          }};
}

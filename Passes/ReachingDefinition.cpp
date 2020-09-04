#include "DFAFramework.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static auto PASS_NAME = "ReachingDefinitionPass";
static auto PASS_VERSION = "v0.1";
static auto ARGUMENT_NAME = "reaching";

class DefInfo {
public:
  /// Interestingly, while the default constructor is never explicitly called,
  /// removing it will result in a compile error
  DefInfo() {}
  DefInfo(std::set<Instruction *> set) : defs(set) {}

  /// Print definition set for a given statement
  /// Called by `print` method of class `DataFlowAnalysis`
  auto print(Bimap<Instruction *, unsigned> &instrMap) -> void {
    for (auto &def : defs) {
      errs() << instrMap.find(def) << " ";
    }
  }

  /// Return true if definition sets have the same definitions
  auto operator==(const DefInfo &other) const -> bool {
    return defs == other.defs;
  }

  /// Meet operator for reaching definition analysis is simply union for
  /// definition set
  auto operator^(const DefInfo &other) const -> DefInfo {
    return DefInfo(set::union2(defs, other.defs));
  }

  std::set<Instruction *> defs;
};

class ReachingDefinitionAnalysis
    : public DataFlowAnalysis<DefInfo, AnalysisDirection::Forward> {
public:
  /// Inherit constructor
  using DataFlowAnalysis::DataFlowAnalysis;

  /// Note that the kill set if always empty due to the SSA property.
  /// Thus the output of the transfer function is the union of input and
  /// Definition generated by the current instruction. (if any)
  virtual auto transferFunction(Instruction *instr, DefInfo input) -> DefInfo {
    // Input is passed in by value, won't change the `in` set
    if (!noRetValue(*instr)) {
      input.defs.insert(instr);
    }
    return input;
  }
};

namespace {
struct ReachingDefinitionPass : public PassInfoMixin<ReachingDefinitionPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    // Instantiate reaching definition analysis with 'top' value of lattice
    auto analysis = ReachingDefinitionAnalysis({}, F);

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

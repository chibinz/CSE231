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

  auto print(Bimap<Instruction*, unsigned> &instrMap) -> void {
    for (auto &def : defs) {
      errs() << instrMap.find(def) << " ";
    }
  }

  auto operator==(const DefInfo &other) const -> bool {
    return defs == other.defs;
  }

  auto operator^(const DefInfo &other) const -> DefInfo {
    return DefInfo(set::union2(defs, other.defs));
  }

  std::set<Instruction *> defs;
};

class ReachingDefinitionAnalysis : public DataFlowAnalysis<DefInfo, true> {
public:
  // Inherit constructor
  using DataFlowAnalysis::DataFlowAnalysis;

  virtual auto transferFunction(Instruction *instr, DefInfo input) -> DefInfo {
    return DefInfo(set::union2(input.defs, {instr}));
  }
};

namespace {
struct ReachingDefinitionPass : public PassInfoMixin<ReachingDefinitionPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
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

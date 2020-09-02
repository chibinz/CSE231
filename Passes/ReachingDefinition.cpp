#include "DFAFramework.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static auto PASS_NAME = "ReachingDefinitionPass";
static auto PASS_VERSION = "v0.1";
static auto ARGUMENT_NAME = "reaching";

class DefInfo : public Info {
public:
  DefInfo() {}
  DefInfo(std::set<Instruction *> set) : defs(set) {}
  virtual ~DefInfo() {}
  virtual auto dump() -> std::string { return ""; }

  static auto equal(const DefInfo *a, const DefInfo *b) -> bool {
    return a->defs == b->defs;
  }

  static auto meet(const DefInfo *a, const DefInfo *b) -> DefInfo * {
    return new DefInfo(set::union2(a->defs, b->defs));
  }

  std::set<Instruction *> defs;
};

class ReachingDefinitionAnalysis : public DataFlowAnalysis<DefInfo, true> {
public:
  ReachingDefinitionAnalysis(DefInfo *l, Function &F)
      : DataFlowAnalysis(l, F) {}

  virtual ~ReachingDefinitionAnalysis() {}

  virtual auto print() -> void {
    auto map = indexInstrs(func);
    for (auto &BB : func) {
      for (auto &I : BB) {
        errs() << map.find(&I) << "\t: ";
        I.print(errs());
        errs() << "\n" << "defs" << "\t: ";
        for (auto &defs: out[&I]->defs) {
          errs() << map.find(defs) << " ";
        }
        errs() << "\n";
      }
    }
  }

  virtual auto transferFunction(Instruction *instr, DefInfo *input)
      -> DefInfo * {
    std::set<Instruction *> new_set = {instr};
    return new DefInfo(set::union2(input->defs, new_set));
  }
};

namespace {
struct ReachingDefinitionPass : public PassInfoMixin<ReachingDefinitionPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    auto top = new DefInfo(std::set<Instruction *>());
    auto analysis = ReachingDefinitionAnalysis(top, F);
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

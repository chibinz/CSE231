#include "DFAFramework.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static auto PASS_NAME = "ConstantPropPass";
static auto PASS_VERSION = "v0.1";
static auto ARGUMENT_NAME = "constprop";

class ConstInfo {
public:
  ConstInfo() {}
  ConstInfo(std::set<Instruction *> set) : consts(set) {}

  auto print(Bimap<Instruction *, unsigned> &instrMap) -> void {
    for (auto &def : consts) {
      errs() << instrMap[def] << " ";
    }
  }

  auto operator==(const ConstInfo &other) const -> bool {
    return consts == other.consts;
  }

  auto operator^(const ConstInfo &other) const -> ConstInfo {
    return ConstInfo(set::union2(consts, other.consts));
  }

  std::set<Value *> consts;
};

class ConstantPropAnalysis
    : public DataFlowAnalysis<ConstInfo, AnalysisDirection::Forward> {
public:
  using DataFlowAnalysis::DataFlowAnalysis;

  virtual auto transferFunction(Instruction *instr, ConstInfo input)
      -> ConstInfo {
    if (!noRetValue(*instr)) {
      input.consts.insert(instr);
    }
    return input;
  }
};

namespace {
struct ConstantPropPass : public PassInfoMixin<ConstantPropPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    auto analysis = ConstantPropAnalysis({}, F);

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
                    FPM.addPass(ConstantPropPass());
                    return true;
                  } else {
                    return false;
                  }
                });
          }};
}

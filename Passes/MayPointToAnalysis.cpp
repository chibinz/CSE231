#include "DFAFramework.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static auto PASS_NAME = "MayPointToAnalysis";
static auto PASS_VERSION = "v0.1";
static auto ARGUMENT_NAME = "maypointto";

class PtrInfo {
public:
  /// Interestingly, while the default constructor is never explicitly called,
  /// removing it will result in a compile error
  PtrInfo() {}
  PtrInfo(std::map<Value *, std::set<Value *>> map) : ptr2val(map) {}

  /// Print definition set for a given statement
  /// Called by `print` method of class `DataFlowAnalysis`
  auto print(Bimap<Instruction *, unsigned> &) -> void {
    for (auto &[p, vs] : ptr2val) {
      p->printAsOperand(errs());
      errs() << ":";
      for (auto v : vs) {
        errs() << "\n";
        v->printAsOperand(errs());
      }
      errs() << "\n";
    }
  }

  /// Return true if definition sets have the same definitions
  auto operator==(const PtrInfo &other) const -> bool {
    return ptr2val == other.ptr2val;
  }

  /// Meet operator for reaching definition analysis is simply union for
  /// definition set
  auto operator^(const PtrInfo &other) const -> PtrInfo {
    // Make a copy of our own ptr2val map
    auto result = ptr2val;
    for (const auto &[p, v] : other.ptr2val) {
      if (result.find(p) != result.end()) {
        result[p] = set::union2(result[p], v);
      } else {
        result[p] = v;
      }
    }

    return result;
  }

  auto add_ptr_alias(Value *ptr, Value *alias) {
    if (ptr2val.find(ptr) != ptr2val.end()) {
      if (ptr2val.find(alias) != ptr2val.end()) {
        ptr2val[alias] = set::union2(ptr2val[ptr], ptr2val[alias]);
      } else {
        ptr2val[alias] = ptr2val[ptr];
      }
    }
  }

  std::map<Value *, std::set<Value *>> ptr2val;
};

class MayPointToAnalysis
    : public DataFlowAnalysis<PtrInfo, AnalysisDirection::Forward> {
public:
  /// Inherit constructor
  using DataFlowAnalysis::DataFlowAnalysis;

  virtual auto transferFunction(Instruction *instr, PtrInfo input) -> PtrInfo {
    auto &map = input.ptr2val;
    switch (instr->getOpcode()) {
    case Instruction::Alloca: {
      map[instr] = {instr->getOperand(0)};
      break;
    }
    case Instruction::BitCast:
    case Instruction::GetElementPtr: {
      input.add_ptr_alias(instr->getOperand(0), instr);
      break;
    }
    case Instruction::Load: {
      auto ptr = instr->getOperand(0);
      if (map.find(ptr) != map.end()) {
        for (auto &x : map[ptr]) {
          input.add_ptr_alias(x, instr);
        }
      }
      break;
    }
    case Instruction::Store: {
      auto val = instr->getOperand(0);
      auto ptr = instr->getOperand(1);
      if (map.find(val) != map.end() && map.find(ptr) != map.end()) {
        auto copy = map[ptr];
        // The following loop might alter map[ptr], a temporary fix
        for (auto &y: copy) {
          input.add_ptr_alias(val, y);
        }
      }
      break;
    }
    case Instruction::Select: {
      input.add_ptr_alias(instr->getOperand(1), instr);
      input.add_ptr_alias(instr->getOperand(2), instr);
      break;
    }
    case Instruction::PHI: {
      for (auto &op : instr->operands()) {
        input.add_ptr_alias(op, instr);
      }
      break;
    }
    default:
      errs() << instr->getOpcodeName() << " ignored!"
             << "\n";
    }

    return input;
  }
};

namespace {
struct ReachingDefinitionPass : public PassInfoMixin<ReachingDefinitionPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    // Instantiate reaching definition analysis with 'top' value of lattice
    auto analysis = MayPointToAnalysis({}, F);

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

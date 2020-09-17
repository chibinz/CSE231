#include "DFAFramework.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static auto PASS_NAME = "MayPointToAnalysis";
static auto PASS_VERSION = "v0.1";
static auto ARGUMENT_NAME = "";

class PtrInfo {
public:
  /// Interestingly, while the default constructor is never explicitly called,
  /// removing it will result in a compile error
  PtrInfo() {}
  PtrInfo(std::set<std::pair<Value *, Value *>> set) : ptr2val(set) {}

  /// Print definition set for a given statement
  /// Called by `print` method of class `DataFlowAnalysis`
  auto print(Bimap<Instruction *, unsigned> &) -> void {}

  /// Return true if definition sets have the same definitions
  auto operator==(const PtrInfo &other) const -> bool {
    return ptr2val == other.ptr2val;
  }

  /// Meet operator for reaching definition analysis is simply union for
  /// definition set
  auto operator^(const PtrInfo &other) const -> PtrInfo {
    return PtrInfo(set::union2(ptr2val, other.ptr2val));
  }


  std::set<std::pair<Value *, Value *>> ptr2val;
};

class MayPointToAnalysis
    : public DataFlowAnalysis<PtrInfo, AnalysisDirection::Backward> {
public:
  /// Inherit constructor
  using DataFlowAnalysis::DataFlowAnalysis;

  virtual auto transferFunction(Instruction *instr, PtrInfo input) -> PtrInfo {
    auto &set = input.ptr2val;
    switch (instr->getOpcode()) {
    case Instruction::Alloca: {
      set.insert({instr, instr});
      break;
    }
    case Instruction::BitCast:
    case Instruction::GetElementPtr: {
      auto value = instr->getOperand(0);
      for (auto &[p, v] : set) {
        if (p == value) {
          set.insert({instr, v});
        }
      }
      break;
    }
    case Instruction::Load: {
      auto ptr = instr->getOperand(0);
      auto pointee = std::set<Value *>();
      for (auto &[p, v] : set) {
        if (p == ptr) {
          pointee.insert(v);
        }
      }
      for (auto &[p, v] : set) {
        if (pointee.find(p) != pointee.end()) {
          set.insert({instr, v});
        }
      }
      break;
    }
    case Instruction::Store: {
      auto val = instr->getOperand(0);
      auto ptr = instr->getOperand(1);
      auto xs = std::set<Value *>();
      for (auto &[p, v] : set) {
        if (p == val) {
          xs.insert(v);
        }
      }
      for (auto &[p, v] : set) {
        if (p == ptr) {
          for (auto x : xs) {
            set.insert({v, x});
          }
        }
      }
      break;
    }
    case Instruction::Select: {
      auto val1 = instr->getOperand(1);
      auto val2 = instr->getOperand(2);
      for (auto &[p, v] : set) {
        if (p == val1 || p == val2) {
          set.insert({instr, v});
        }
      }
      break;
    }

    case Instruction::PHI: {
      for (auto &op : instr->operands()) {
        for (auto &[p, v] : set) {
          if (p == op) {
            set.insert({instr, v});
          }
        }
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

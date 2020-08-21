#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

static char PASS_NAME[] = "BasePass";
static char PASS_VERSION[] = "v0.1";
static char ARGUMENT_NAME[] = "base-pass";

/// All the tutorials I've seen seems to put their own pass
/// in an anonymous namespace
namespace {
/// Struct name of your pass and the template parameter should be identical
struct BasePass : public PassInfoMixin<BasePass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    // Now that you have a reference to a function,
    // do whatever you want to with it.
    return PreservedAnalyses::none();
  }
};
} // namespace

/// The callback function's the first and the third parameter
/// stays the same, while the second parameter comes in 4 flavors:
/// CGSCCPassManager, FunctionPassManager, LoopPassManager, ModulePassManager
bool CallBackFunction(StringRef Name, FunctionPassManager &FPM,
                      ArrayRef<PassBuilder::PipelineElement>) {
  /// Check if the command argument passed is same as the specified one.
  /// The `==` operand here is probably overloaded for string comparison.
  if (Name == ARGUMENT_NAME) {
    FPM.addPass(BasePass());
    return true;
  } else {
    return false;
  }
}

/// Usually, the registration function and callback function are inlined
/// as lambda functions. Here it is expanded for the sake of clarity.
void RegistrationFunction(PassBuilder &PB) {
  PB.registerPipelineParsingCallback(CallBackFunction);
}

/// Weird line of code, but seems necessary to get it compiling.
/// `PassPlugin.h` in llvm source contains similar lines of code.
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  /// Struct name omitted ...?
  return {LLVM_PLUGIN_API_VERSION, PASS_NAME, PASS_VERSION,
          RegistrationFunction};
}

/// Struct definition copy pasted from llvm header files
/*
struct PassPluginLibraryInfo {
  /// The API version understood by this plugin, usually \c
  /// LLVM_PLUGIN_API_VERSION
  uint32_t APIVersion;
  /// A meaningful name of the plugin.
  const char *PluginName;
  /// The version of the plugin.
  const char *PluginVersion;

  /// The callback for registering plugin passes with a \c PassBuilder
  /// instance
  void (*RegisterPassBuilderCallbacks)(PassBuilder &);
};
*/
project('CSE231', 'c', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=c11', 'cpp_std=c++17'])

llvm_dep = dependency('llvm')

shared_library('CountStaticInstructions', 'Passes/CountStaticInstructions.cpp', dependencies: llvm_dep)
shared_library('CountDynamicInstructions', 'Passes/CountDynamicInstructions.cpp', dependencies: llvm_dep)
shared_library('BranchBias', 'Passes/BranchBias.cpp', dependencies: llvm_dep)
shared_library('ReachingDefinition', 'Passes/ReachingDefinition.cpp', dependencies: llvm_dep)
shared_library('LiveVariable', 'Passes/LiveVariable.cpp', dependencies: llvm_dep)
shared_library('MayPointToAnalysis', 'Passes/MayPointToAnalysis.cpp', dependencies: llvm_dep)
shared_library('ConstantPropAnalysis', 'Passes/ConstantPropAnalysis.cpp', dependencies: llvm_dep)
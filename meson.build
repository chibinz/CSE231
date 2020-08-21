project('CSE231', 'c', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=c11', 'cpp_std=c++17'])

llvm_dep = dependency('llvm')

shared_library('BasePass', 'Passes/BasePass.cpp', dependencies: llvm_dep)

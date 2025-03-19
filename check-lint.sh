#!/bin/bash

# Run clang-tidy on all .cpp and .h files in the src directory
clang-tidy -checks="*" -- -Iinclude $(find src -name "*.cpp" -o -name "*.h")

# If you want to exclude certain checks, modify the checks list, e.g.:
# clang-tidy -checks="-clang-analyzer-*,-llvm-*" -- -Iinclude $(find src -name "*.cpp" -o -name "*.h")

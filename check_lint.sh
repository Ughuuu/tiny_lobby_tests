#!/bin/bash

clang-tidy -checks="*" -- -Iinclude $(find src -name "*.cpp" -o -name "*.h")

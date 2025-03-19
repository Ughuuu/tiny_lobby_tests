#!/bin/bash

find src -name "*.cpp" -o -name "*.h" | xargs clang-tidy -checks="*" -- -Iinclude

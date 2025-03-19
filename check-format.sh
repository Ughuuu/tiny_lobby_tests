#!/bin/bash

# Find all .cpp and .h files in the src directory and check them using clang-format
UNFORMATTED_FILES=$(find src -name "*.cpp" -o -name "*.h" | xargs clang-format --style=file --dry-run --Werror)

if [ -n "$UNFORMATTED_FILES" ]; then
    echo "The following files are not correctly formatted:"
    echo "$UNFORMATTED_FILES"
    exit 1
fi

echo "All files are correctly formatted!"
exit 0

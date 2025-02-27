#!/bin/bash

# Script to build and execute the concurrent hash map test until failure.
# Will stop when any non-zero exit code is encountered.
# Press Ctrl+C to stop manually.

echo "Starting loop - will run until a command fails"
echo "Press Ctrl+C to stop manually"

counter=1

# Exit if compilation fails
if ! clang++ -g -O3 -Wall -Wextra -pedantic -Werror -std=c++20 -pthread main.cpp -fsanitize=thread; then
  exit_code=$?
  echo "Compilation failed with exit code $exit_code"
  echo "Stopping after $counter iterations"
  exit $exit_code
fi

echo "Compilation successful, running executable..."
while true; do
  echo "-------------------------------------------"
  echo "Iteration #$counter"
  echo "-------------------------------------------"

  if time ./a.out; then
    echo "Execution successful"
  else
    exit_code=$?
    echo "Execution failed with exit code $exit_code"
    echo "Stopping after $counter iterations"
    break
  fi
  counter=$((counter + 1))
  echo ""
done

echo "Loop complete - non-zero exit code detected"

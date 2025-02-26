#!/bin/bash

# Script to run a build and execution command until failure
# Will stop when any non-zero exit code is encountered

echo "Starting loop - will run until a command fails"
echo "Press Ctrl+C to stop manually"

counter=1

while true; do
  echo "-------------------------------------------"
  echo "Iteration #$counter"
  echo "-------------------------------------------"

  # Run the compile command and if successful, run the program with time
  if clang++ -g -O3 -Wall -Wextra -pedantic -Werror -std=c++20 -pthread main.cpp; then
    echo "Compilation successful, running executable..."
    if time ./a.out; then
      echo "Execution successful"
    else
      exit_code=$?
      echo "Execution failed with exit code $exit_code"
      echo "Stopping after $counter iterations"
      break
    fi
  else
    exit_code=$?
    echo "Compilation failed with exit code $exit_code"
    echo "Stopping after $counter iterations"
    break
  fi

  counter=$((counter + 1))
  echo ""
done

echo "Loop complete - non-zero exit code detected"

#!/bin/bash

# Default values for arguments
FILTER=""
ITERATIONS=0

# Usage function to display script options
usage() {
    echo "Usage: $0 -i <iterations> [-f <filter>]"
    exit 1
}

# Parse arguments
while getopts ":f:i:" opt; do
  case $opt in
    f)
      FILTER="$OPTARG"
      ;;
    i)
      ITERATIONS="$OPTARG"
      ;;
    *)
      usage
      ;;
  esac
done

# Check if iterations argument is provided
if [ $ITERATIONS -le 0 ]; then
    echo "Error: Please provide a valid number for iterations (-i <n>)"
    usage
fi

# Create filter string for the command
FILTER_CMD=""
if [ -n "$FILTER" ]; then
    FILTER_CMD="--filter $FILTER"
fi

# Run the tests for n iterations or until failure
for (( i=1; i<=ITERATIONS; i++ )); do
    echo "Running iteration $i with filter: $FILTER_CMD"
    
    # Run the test with the specified filter
    bin/pbx_tests $FILTER_CMD -j1
    if [ $? -ne 0 ]; then
        echo "Test failed during iteration $i. Exiting..."
        exit 1
    fi
done

echo "All $ITERATIONS iterations passed successfully."

#!/bin/bash

# USAGE: ./run_parallel.sh
# Optional: modify TESTCASES or FLAGS_ARRAY

# Set max number of parallel jobs
MAX_JOBS=4

# Your testcase name
TESTCASE="ethmac"

# List of different config flags to test
FLAGS_ARRAY=(
  ""
  "-timing"
  "-cellswap"
  "-cellswap -timing"
)

# Track background job PIDs
pids=()

# Function to check and wait for jobs if exceeding limit
function wait_for_jobs {
  while [ $(jobs -rp | wc -l) -ge $MAX_JOBS ]; do
    sleep 1
  done
}

# Main loop: Launch jobs
for flags in "${FLAGS_ARRAY[@]}"; do
  wait_for_jobs
  echo "Launching: ./run.sh $TESTCASE $flags"
  ./run.sh "$TESTCASE" $flags &
  pids+=($!)
done

# Wait for all jobs to finish
wait
echo "🎉 All runs completed."

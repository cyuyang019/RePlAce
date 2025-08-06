#!/bin/bash

set -e
set -o pipefail  # Make pipeline return non-zero if any command fails

# Usage: ./run.sh <testcase_name> [extra_flags...]
if [ -z "$1" ]; then
    echo "Usage: ./run.sh <testcase_name> [extra_flags...]"
    exit 1
fi

TESTCASE=$1
shift  # Remove the first argument (testcase name), keep the rest as extra flags

# Convert extra flags into a safe string: keep dashes, replace spaces with underscores
FLAGS_STR=$(echo "$@" | tr ' ' '_' | tr -d '-')

# Add timestamp in YYYY-MM-DD_HH-MM-SS format
export TZ="Asia/Taipei"
TIMESTAMP=$(date +"%m%d%H%M")

# Combine everything for the log filename
LOGFILE="Log/${TESTCASE}${FLAGS_STR:+_$FLAGS_STR}_$TIMESTAMP.log"

# Run the main binary
./build/replace \
    -3D \
    -cadbin ./test/${TESTCASE}/${TESTCASE}_pl.in \
    -cadbglobal ./test/${TESTCASE}/${TESTCASE}_pl.global \
    -lib ./test/${TESTCASE}/NanGate_3D_slow.lib \
    -verilog ./test/${TESTCASE}/${TESTCASE}_3D.v \
    -sdc ./test/${TESTCASE}/${TESTCASE}.sdc \
    -config ./test/${TESTCASE}/config.json \
    -legalize \
    "$@" \
    -output ./TTest 2>&1 | tee "$LOGFILE"

EXIT_CODE=$?  # Works now with pipefail

# Rename based on success or failure
if [ $EXIT_CODE -eq 0 ]; then
    NEWFILE="${LOGFILE%.log}_SUCCESS.log"
    echo "✅ SUCCESS: Log saved to $NEWFILE"
else
    NEWFILE="${LOGFILE%.log}_FAIL.log"
    echo "❌ ERROR: Log saved to $NEWFILE"
fi

mv "$LOGFILE" "$NEWFILE"
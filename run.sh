#!/bin/bash

# Usage: ./run.sh <testcase_name> [extra_flags...]
if [ -z "$1" ]; then
    echo "Usage: ./run.sh <testcase_name> [extra_flags...]"
    exit 1
fi

TESTCASE=$1
shift  # Remove the first argument (testcase name), keep the rest as extra flags

./build/replace \
    -3D \
    -cadbin ./test/${TESTCASE}/${TESTCASE}_pl.in \
    -cadbglobal ./test/${TESTCASE}/${TESTCASE}_pl.global \
    -lib ./test/${TESTCASE}/NanGate_3D_slow.lib \
    -verilog ./test/${TESTCASE}/${TESTCASE}_3D.v \
    -sdc ./test/${TESTCASE}/${TESTCASE}.sdc \
    -legalize \
    "$@" \
    -output ./TTest
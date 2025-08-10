#!/bin/bash

# Default values
DELAY=20
FINAL_DELAY=100
OUTPUT="animated.gif"

# Argument parser
while [[ $# -gt 0 ]]; do
    case "$1" in
        --case)
            CASE="$2"
            shift 2
            ;;
        --exp)
            EXP_NUM_RAW="$2"
            shift 2
            ;;
        --tier)
            TIER="$2"
            shift 2
            ;;
        --output)
            OUTPUT="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 --case <name> --exp <num> --tier <num> [--output <filename>]"
            exit 1
            ;;
    esac
done

# Validate inputs
if [[ -z "$CASE" || -z "$EXP_NUM_RAW" || -z "$TIER" ]]; then
    echo "Error: --case, --exp, and --tier are required."
    echo "Usage: $0 --case <name> --exp <num> --tier <num> [--output <filename>]"
    exit 1
fi

# Zero-pad experiment number to 3 digits
EXP=$(printf "%03d" "$EXP_NUM_RAW")

# Construct the directory path
DIR="./TTest/${CASE}_pl/experiment${EXP}/cell/tier${TIER}"

# Find and sort frames
frames=($(ls "${DIR}/iter"*.jpg 2>/dev/null | sort -V))

if [[ ${#frames[@]} -eq 0 ]]; then
    echo "Error: No frames found in ${DIR}"
    exit 1
fi

# Separate last frame
last="${frames[-1]}"
unset 'frames[-1]'

# Create the GIF
convert -delay "$DELAY" "${frames[@]}" -delay "$FINAL_DELAY" "$last" -loop 0 "$OUTPUT"

echo "✅ GIF created: $OUTPUT"

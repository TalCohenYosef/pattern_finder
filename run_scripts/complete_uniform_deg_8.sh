#!/bin/bash

# Configuration to complete input_color_uniform_deg_8
INPUT_DIR="input_color_uniform_deg_8"
OUTPUT_DIR="OUTPUT_${INPUT_DIR}"
PATTERNS_DIR="${OUTPUT_DIR}/patterns"
LOGS_DIR="${OUTPUT_DIR}/logs"

# Check existing patterns
EXISTING_PATTERNS=$(ls "$PATTERNS_DIR"/P_*.json 2>/dev/null | wc -l)
START_PATTERN=$((EXISTING_PATTERNS + 1))
TOTAL_PATTERNS=350

echo "Completing patterns for ${INPUT_DIR}..."
echo "Existing patterns: $EXISTING_PATTERNS"
echo "Starting from pattern: $START_PATTERN"
echo "Need to create: $((TOTAL_PATTERNS - EXISTING_PATTERNS)) more patterns"
echo "========================================"

# Create directories if they don't exist
mkdir -p "$PATTERNS_DIR"
mkdir -p "$LOGS_DIR"

# Total timing
TOTAL_START=$(date +%s.%N)

# Continue from where we left off
for ((i=START_PATTERN; i<=TOTAL_PATTERNS; i++)); do
    echo "Running pattern $i/$TOTAL_PATTERNS..."
    
    # Timing for this pattern
    PATTERN_START=$(date +%s.%N)
    
    # Run the pattern generation
    ./pattern_finder --path "${INPUT_DIR}" --alive 0.005 "${i}" > "${LOGS_DIR}/run_${i}_output.log" 2>&1
    
    # Copy pattern.json to the correct location if it was created
    if [ -f "pattern.json" ]; then
        cp "pattern.json" "${PATTERNS_DIR}/P_${i}.json"
        rm "pattern.json"  # Clean up
    fi
    
    # Check if successful
    PATTERN_END=$(date +%s.%N)
    PATTERN_DURATION=$(echo "$PATTERN_END - $PATTERN_START" | bc -l)
    
    if [ $? -eq 0 ]; then
        echo "  Run $i: SUCCESS (${PATTERN_DURATION}s)"
        
        # Update PATTERN_SUMMARY.log
        echo "Run $i: SUCCESS (${PATTERN_DURATION}s)" >> "${OUTPUT_DIR}/PATTERN_SUMMARY.log"
    else
        echo "  Run $i: FAILED (${PATTERN_DURATION}s)"
        
        # Update PATTERN_SUMMARY.log
        echo "Run $i: FAILED (${PATTERN_DURATION}s)" >> "${OUTPUT_DIR}/PATTERN_SUMMARY.log"
    fi
    
    # Show progress every 10 patterns
    if [ $((i % 10)) -eq 0 ]; then
        echo "  Progress: $i/$TOTAL_PATTERNS patterns completed"
    fi
done

TOTAL_END=$(date +%s.%N)
TOTAL_DURATION=$(echo "$TOTAL_END - $TOTAL_START" | bc -l)

echo ""
echo "========================================"
echo "Pattern generation completed!"
echo "========================================"
echo "Total time: ${TOTAL_DURATION} seconds"
echo "Average time per pattern: $(echo "scale=6; $TOTAL_DURATION / ($TOTAL_PATTERNS - START_PATTERN + 1)" | bc -l) seconds"
echo "Total patterns created: $(ls "$PATTERNS_DIR"/P_*.json 2>/dev/null | wc -l)"

# Create final summary
echo ""
echo "Final summary created in: ${OUTPUT_DIR}/PATTERN_SUMMARY.log"
echo "All patterns saved in: $PATTERNS_DIR"
echo "All logs saved in: $LOGS_DIR"

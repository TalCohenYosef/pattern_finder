#!/bin/bash

# Array of different input directories
INPUT_DIRS=(
    "input_color_uniform_deg_8"
    "input_color_uniform_deg_15"
)

# Find which directory to continue from
START_DIR=""
START_PATTERN=1

for INPUT_DIR in "${INPUT_DIRS[@]}"; do
    OUTPUT_DIR="OUTPUT_${INPUT_DIR}"
    PATTERNS_DIR="$OUTPUT_DIR/patterns"
    
    if [ ! -d "$OUTPUT_DIR" ]; then
        echo "Directory $OUTPUT_DIR doesn't exist, starting from here"
        START_DIR="$INPUT_DIR"
        START_PATTERN=1
        break
    fi
    
    # Check how many patterns exist
    if [ -d "$PATTERNS_DIR" ]; then
        EXISTING_PATTERNS=$(ls "$PATTERNS_DIR"/P_*.json 2>/dev/null | wc -l)
        if [ "$EXISTING_PATTERNS" -lt 350 ]; then
            echo "Directory $OUTPUT_DIR has $EXISTING_PATTERNS/350 patterns, continuing from here"
            START_DIR="$INPUT_DIR"
            START_PATTERN=$((EXISTING_PATTERNS + 1))
            break
        fi
    else
        echo "Patterns directory $PATTERNS_DIR doesn't exist, starting from here"
        START_DIR="$INPUT_DIR"
        START_PATTERN=1
        break
    fi
done

if [ -z "$START_DIR" ]; then
    echo "All directories are complete!"
    exit 0
fi

echo "========================================"
echo "CONTINUING FROM: $START_DIR"
echo "Starting pattern: $START_PATTERN"
echo "========================================"

# Find the index of the starting directory
START_INDEX=0
for i in "${!INPUT_DIRS[@]}"; do
    if [ "${INPUT_DIRS[$i]}" = "$START_DIR" ]; then
        START_INDEX=$i
        break
    fi
done

# Continue from the found directory
for ((i=START_INDEX; i<${#INPUT_DIRS[@]}; i++)); do
    INPUT_DIR="${INPUT_DIRS[$i]}"
    
    # Set starting pattern for first directory, 1 for others
    if [ $i -eq $START_INDEX ]; then
        CURRENT_START=$START_PATTERN
    else
        CURRENT_START=1
    fi
    
    echo "========================================"
    echo "Processing input directory: $INPUT_DIR"
    echo "Starting from pattern: $CURRENT_START"
    echo "========================================"
    
    # Create directories for this input
    OUTPUT_DIR="OUTPUT_${INPUT_DIR}"
    PATTERNS_DIR="$OUTPUT_DIR/patterns"
    LOGS_DIR="$OUTPUT_DIR/logs"
    mkdir -p "$OUTPUT_DIR"
    mkdir -p "$PATTERNS_DIR"
    mkdir -p "$LOGS_DIR"
    
    # Total timing for this input
    TOTAL_START=$(date +%s.%N)
    
    if [ $CURRENT_START -eq 1 ]; then
        echo "Starting 350 pattern finding runs for $INPUT_DIR..."
        END_PATTERN=350
    else
        echo "Continuing pattern finding for $INPUT_DIR from $CURRENT_START to 350..."
        END_PATTERN=350
    fi
    
    # Run pattern_finder from CURRENT_START to 350
    for ((j=CURRENT_START; j<=END_PATTERN; j++)); do
        echo "Run $j/350"
        
        # Time the pattern finding
        START_TIME=$(date +%s.%N)
        
        # Run pattern_finder with current input directory and capture output
        ./pattern_finder --path "$INPUT_DIR/" --alive 0.005 > "$LOGS_DIR/run_${j}_output.log" 2>&1
        
        END_TIME=$(date +%s.%N)
        DURATION=$(echo "$END_TIME - $START_TIME" | bc -l)
        
        # Save pattern with run number
        if [ -f "pattern.json" ]; then
            cp pattern.json "$PATTERNS_DIR/P_${j}.json"
            
            # Extract pattern appearance info
            PATTERN_INFO=$(grep "Pattern appears in" "$LOGS_DIR/run_${j}_output.log")
            
            echo "Run $j: SUCCESS (${DURATION}s) - $PATTERN_INFO" >> "$OUTPUT_DIR/PATTERN_SUMMARY.log"
            echo "  ✓ P_${j}.json saved (${DURATION}s)"
        else
            echo "  ✗ No pattern.json generated"
            echo "Run $j: FAILED - No pattern generated" >> "$OUTPUT_DIR/PATTERN_SUMMARY.log"
        fi
        
        # Clean up temporary pattern file
        rm -f pattern.json
    done
    
    TOTAL_END=$(date +%s.%N)
    TOTAL_DURATION=$(echo "$TOTAL_END - $TOTAL_START" | bc -l)
    
    echo ""
    echo "Pattern finding completed for $INPUT_DIR!"
    echo "Total time: ${TOTAL_DURATION} seconds"
    
    # Also save final results to the pattern_summary.log file
    echo "" >> "$OUTPUT_DIR/PATTERN_SUMMARY.log"
    echo "FINAL RESULTS for $INPUT_DIR" >> "$OUTPUT_DIR/PATTERN_SUMMARY.log"
    echo "========================================" >> "$OUTPUT_DIR/PATTERN_SUMMARY.log"
    echo "Total pattern finding time: ${TOTAL_DURATION} seconds" >> "$OUTPUT_DIR/PATTERN_SUMMARY.log"
    echo "Average time per pattern: $(echo "scale=6; $TOTAL_DURATION / 350" | bc -l) seconds" >> "$OUTPUT_DIR/PATTERN_SUMMARY.log"
    
    echo "Results saved in: $OUTPUT_DIR/"
    echo ""
done

echo "========================================"
echo "All input directories completed!"
echo "========================================"

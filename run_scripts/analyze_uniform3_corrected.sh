#!/bin/bash

echo "Analyzing pattern matching results for average_deg_3..."
echo "======================================================"

# Configuration - FIXED PATHS
PATTERNS_DIR="/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results/ OUTPUT_input_average_deg_3"
LOGS_DIR="/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results/ OUTPUT_input_average_deg_3/logs"
OUTPUT_DIR="/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results/ OUTPUT_input_average_deg_3"
SUBGRAPH_DIR="/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/Graph-Search"

# Target graphs
TARGET_GRAPHS=(
   /home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/local_tests/graphs_by_density_3/g_den_8_embedded_den_3_average_0.json
)

# Initialize variables
TOTAL_START=$(date +%s.%N)
FOUND_PATTERNS=()
NOT_FOUND_PATTERNS=()
NOT_IN_G_S=()

echo "Patterns directory: $PATTERNS_DIR"
echo "Number of target graphs: ${#TARGET_GRAPHS[@]}"
echo ""

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Test each target graph
for g_index in "${!TARGET_GRAPHS[@]}"; do
    TARGET_GRAPH="${TARGET_GRAPHS[$g_index]}"
    GRAPH_NAME=$(basename "$TARGET_GRAPH" .json)
    
    echo "Processing graph: $GRAPH_NAME"
    GRAPH_START=$(date +%s.%N)
    
    # Reset arrays for this graph
    FOUND_PATTERNS=()
    NOT_FOUND_PATTERNS=()
    NOT_IN_G_S=()
    
    # Check if patterns directory exists
    if [ ! -d "$PATTERNS_DIR" ]; then
        echo "ERROR: Patterns directory not found: $PATTERNS_DIR"
        exit 1
    fi
    
    # Process each pattern
    for pattern_file in "$PATTERNS_DIR"/S_*.json; do
        if [ -f "$pattern_file" ]; then
            PATTERN_NAME=$(basename "$pattern_file" .json)
            
            # Run subgraph test
            "$SUBGRAPH_DIR"/subgraph_isomorphism "$TARGET_GRAPH" "$pattern_file" > /dev/null 2>&1
            EXIT_CODE=$?
            
            if [ $EXIT_CODE -eq 0 ]; then
                FOUND_PATTERNS+=("$PATTERN_NAME")
                echo "$PATTERN_NAME: FOUND in $GRAPH_NAME"
            else
                NOT_FOUND_PATTERNS+=("$PATTERN_NAME")
                NOT_IN_G_S+=("$PATTERN_NAME")
                echo "$PATTERN_NAME: NOT FOUND in $GRAPH_NAME"
            fi
        fi
    done
    
    GRAPH_END=$(date +%s.%N)
    GRAPH_TIME=$(echo "$GRAPH_END - $GRAPH_START" | bc)
    
    echo "Completed $GRAPH_NAME in $GRAPH_TIME seconds"
    echo ""
    
    # Extract unique S numbers, sort them, and save
    UNIQUE_S=($(printf "%s\n" "${NOT_IN_G_S[@]}" | sed 's/S_\([0-9]*\)/\1/' | sort -n | uniq))
    
    # Create results file for this graph
    RESULTS_FILE="$OUTPUT_DIR/RESULTS_${GRAPH_NAME}.log"
    
    # Create directory if it doesn't exist
    mkdir -p "$(dirname "$RESULTS_FILE")"
    
    echo "Total number of S not in G: ${#UNIQUE_S[@]}" > "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
    echo "Total subgraph testing time: $GRAPH_TIME seconds" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
    echo "S numbers not in G (sorted):" >> "$RESULTS_FILE"
    
    for s_num in "${UNIQUE_S[@]}"; do
        echo "$s_num" >> "$RESULTS_FILE"
    done
    
    echo "Results saved in: $RESULTS_FILE"
    echo "S files not in G (unique): ${#UNIQUE_S[@]}"
    echo ""
done

TOTAL_END=$(date +%s.%N)
TOTAL_TIME=$(echo "$TOTAL_END - $TOTAL_START" | bc)

echo "======================================================"
echo "SUMMARY"
echo "======================================================"
echo "Total time: $TOTAL_TIME seconds"
echo "Average time per graph: $(echo "scale=6; $TOTAL_TIME / ${#TARGET_GRAPHS[@]}" | bc) seconds"
echo ""

echo "Creating master summary..."

# Create master summary file
MASTER_SUMMARY="$OUTPUT_DIR/MASTER_SUMMARY_average3_FIXED.log"
mkdir -p "$(dirname "$MASTER_SUMMARY")"

echo "COMPREHENSIVE PATTERN ANALYSIS SUMMARY - average DEG 3 (FIXED)" > "$MASTER_SUMMARY"
echo "=================================================================" >> "$MASTER_SUMMARY"
echo "Date: $(date)" >> "$MASTER_SUMMARY"
echo "Total patterns tested: 300" >> "$MASTER_SUMMARY"
echo "Total target graphs: ${#TARGET_GRAPHS[@]}" >> "$MASTER_SUMMARY"
echo "Total execution time: $TOTAL_TIME seconds" >> "$MASTER_SUMMARY"
echo "" >> "$MASTER_SUMMARY"

for g_index in "${!TARGET_GRAPHS[@]}"; do
    TARGET_GRAPH="${TARGET_GRAPHS[$g_index]}"
    GRAPH_NAME=$(basename "$TARGET_GRAPH" .json)
    RESULTS_FILE="$OUTPUT_DIR/RESULTS_${GRAPH_NAME}.log"
    
    if [ -f "$RESULTS_FILE" ]; then
        echo "Results for $GRAPH_NAME:" >> "$MASTER_SUMMARY"
        echo "------------------------" >> "$MASTER_SUMMARY"
        
        # Extract stats from results file
        NOT_FOUND_COUNT=$(head -1 "$RESULTS_FILE" | grep -o '[0-9]\+')
        TIME=$(grep "Total subgraph testing time" "$RESULTS_FILE" | grep -o '[0-9.]\+')
        
        echo "  S not found: $NOT_FOUND_COUNT" >> "$MASTER_SUMMARY"
        echo "  S found: $((300 - NOT_FOUND_COUNT))" >> "$MASTER_SUMMARY"
        echo "  Testing time: $TIME seconds" >> "$MASTER_SUMMARY"
        echo "" >> "$MASTER_SUMMARY"
    fi
done

echo "All results saved in: $OUTPUT_DIR"
echo "Individual results: RESULTS_*.log"
echo "Master summary: MASTER_SUMMARY_average3_FIXED.log"
echo ""
echo "Fixed pattern analysis for input_color_average_deg_3 completed!"

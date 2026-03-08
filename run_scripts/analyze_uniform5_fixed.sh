#!/bin/bash

echo "Analyzing pattern matching results for uniform_deg_5..."
echo "======================================================"

# Configuration
PATTERNS_DIR="/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results/OUTPUT_input_color_uniform_deg_5/patterns"
LOGS_DIR="/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results/OUTPUT_input_color_uniform_deg_5/logs"
OUTPUT_DIR="/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/results/OUTPUT_input_color_uniform_deg_5"
SUBGRAPH_DIR="/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/Graph-Search"

# Target graphs
TARGET_GRAPHS=(
    "/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/local_tests/graphs_by_density_5/g_den_8_embedded_den_5_uniform_0.json"
    "/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/local_tests/graphs_by_density_5/g_den_10_embedded_den_5_uniform_0.json"
    "/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/local_tests/graphs_by_density_5/g_den_13_embedded_den_5_uniform_0.json"
    "/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/local_tests/graphs_by_density_5/g_den_15_embedded_den_5_uniform_0.json"

)

# Initialize variables
TOTAL_START=$(date +%s.%N)
FOUND_PATTERNS=()
NOT_FOUND_PATTERNS=()
NOT_IN_G_S=()

echo "Patterns directory: $PATTERNS_DIR"
echo "Number of target graphs: ${#TARGET_GRAPHS[@]}"
echo ""

# Test each target graph
for g_index in "${!TARGET_GRAPHS[@]}"; do
    TARGET_GRAPH="${TARGET_GRAPHS[$g_index]}"
    GRAPH_NAME=$(basename "$TARGET_GRAPH" .json)
    
    echo "Testing graph: $GRAPH_NAME"
    GRAPH_START=$(date +%s.%N)
    
    # Test patterns 1-550
    for i in {1..550}; do
        PATTERN_FILE="$PATTERNS_DIR/P_${i}.json"
        
        if [ -f "$PATTERN_FILE" ]; then
            echo -n "Testing P_${i}... "
            
            # Run subgraph isomorphism test
            cd "$SUBGRAPH_DIR"
            timeout 60s ./subgraph_isomorphism "$TARGET_GRAPH" "$PATTERN_FILE" > temp_output.txt 2>&1
            EXIT_CODE=$?
            
            if [ $EXIT_CODE -eq 124 ]; then
                echo "TIMEOUT - assumed FOUND"
                FOUND_PATTERNS+=("P_${i}")
            else
                # Extract matches count
                MATCHES=$(grep "matches" temp_output.txt | tail -1 | grep -o "matches [0-9]*" | grep -o "[0-9]*")
                
                if [ -z "$MATCHES" ]; then
                    MATCHES="0"
                fi
                
                if [ "$MATCHES" -gt 0 ]; then
                    FOUND_PATTERNS+=("P_${i}")
                    echo "FOUND ($MATCHES matches)"
                else
                    NOT_FOUND_PATTERNS+=("P_${i}")
                    
                    # Get S files from this pattern (they are definitely NOT in G)
                    if [ -f "$LOGS_DIR/run_${i}_output.log" ]; then
                        S_IN_PATTERN=$(grep "S\[" "$LOGS_DIR/run_${i}_output.log" | sed 's/.*S_\([0-9]*\)\.json/\1/')
                        for s_num in $S_IN_PATTERN; do
                            NOT_IN_G_S+=("S_${s_num}.json")
                        done
                    fi
                    
                    echo "NOT found"
                fi
            fi
            rm -f temp_output.txt
        else
            echo "File not found"
        fi
    done
    
    GRAPH_END=$(date +%s.%N)
    GRAPH_TIME=$(echo "$GRAPH_END - $GRAPH_START" | bc)
    
    echo "Completed $GRAPH_NAME in $GRAPH_TIME seconds"
    echo ""
    
    # Extract unique S numbers, sort them, and save
    UNIQUE_S=($(printf "%s\n" "${NOT_IN_G_S[@]}" | sed 's/S_\([0-9]*\)\.json/\1/' | sort -n | uniq))
    
    # Create results file for this graph
    RESULTS_FILE="$OUTPUT_DIR/RESULTS_${GRAPH_NAME}.log"
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
MASTER_SUMMARY="$OUTPUT_DIR/MASTER_SUMMARY_UNIFORM5_FIXED.log"
echo "COMPREHENSIVE PATTERN ANALYSIS SUMMARY - UNIFORM DEG 5 (FIXED)" > "$MASTER_SUMMARY"
echo "=================================================================" >> "$MASTER_SUMMARY"
echo "Date: $(date)" >> "$MASTER_SUMMARY"
echo "Total patterns tested: 550" >> "$MASTER_SUMMARY"
echo "Total target graphs: ${#TARGET_GRAPHS[@]}" >> "$MASTER_SUMMARY"
echo "Total execution time: $TOTAL_TIME seconds" >> "$MASTER_SUMMARY"
echo "" >> "$MASTER_SUMMARY"

for g_index in "${!TARGET_GRAPHS[@]}"; do
    TARGET_GRAPH="${TARGET_GRAPHS[$g_index]}"
    GRAPH_NAME=$(basename "$TARGET_GRAPH" .json)
    RESULTS_FILE="$OUTPUT_DIR/RESULTS_${GRAPH_NAME}.log"
    
    if [ -f "$RESULTS_FILE" ]; then
        echo "Results for $GRAPH_NAME:" >> "$MASTER_SUMMARY"
        S_COUNT=$(head -1 "$RESULTS_FILE" | grep -o "[0-9]*")
        echo "  S files not in G: $S_COUNT" >> "$MASTER_SUMMARY"
        TIME_LINE=$(grep "Total subgraph testing time:" "$RESULTS_FILE")
        echo "  $TIME_LINE" >> "$MASTER_SUMMARY"
        echo "" >> "$MASTER_SUMMARY"
    fi
done

echo "All results saved in: $OUTPUT_DIR"
echo "Individual results: RESULTS_*.log"
echo "Master summary: MASTER_SUMMARY_UNIFORM5_FIXED.log"
echo ""
echo "Fixed pattern analysis for input_color_uniform_deg_5 completed!"

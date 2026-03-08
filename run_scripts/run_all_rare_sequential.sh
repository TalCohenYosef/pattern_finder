#!/bin/bash

echo "Running all rare analysis scripts sequentially with nohup..."
echo "============================================================"

# List of scripts to run
SCRIPTS=(
    "/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/run_scripts/analyze_rare3_fixed.sh"
    "/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/run_scripts/analyze_rare5_fixed.sh"
    "/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/run_scripts/analyze_rare8_fixed.sh"
    "/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/run_scripts/analyze_rare15_fixed.sh"
)

# Total start time
TOTAL_START=$(date +%s.%N)
MAIN_LOG="/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/run_scripts/all_rare_sequential.log"

echo "Main log file: $MAIN_LOG"
echo ""

for i in "${!SCRIPTS[@]}"; do
    script="${SCRIPTS[$i]}"
    script_name=$(basename "$script" .sh)
    script_log="/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/run_scripts/${script_name}_output.log"
    
    if [ -f "$script" ]; then
        echo "==================================================" | tee -a "$MAIN_LOG"
        echo "Running: $(basename "$script")" | tee -a "$MAIN_LOG"
        echo "Log file: $script_log" | tee -a "$MAIN_LOG"
        echo "==================================================" | tee -a "$MAIN_LOG"
        
        # Run the script with nohup and wait for it to complete
        SCRIPT_START=$(date +%s.%N)
        
        # Use nohup to ensure the script continues running even if disconnected
        nohup bash "$script" > "$script_log" 2>&1 &
        SCRIPT_PID=$!
        
        echo "Started with PID: $SCRIPT_PID" | tee -a "$MAIN_LOG"
        echo "Waiting for completion..." | tee -a "$MAIN_LOG"
        
        # Wait for the script to complete
        while kill -0 $SCRIPT_PID 2>/dev/null; do
            sleep 10
            echo -n "." | tee -a "$MAIN_LOG"
        done
        
        SCRIPT_END=$(date +%s.%N)
        SCRIPT_TIME=$(echo "$SCRIPT_END - $SCRIPT_START" | bc)
        
        # Check if script completed successfully
        wait $SCRIPT_PID
        EXIT_CODE=$?
        
        echo "" | tee -a "$MAIN_LOG"
        if [ $EXIT_CODE -eq 0 ]; then
            echo "✅ Completed successfully in $SCRIPT_TIME seconds" | tee -a "$MAIN_LOG"
        else
            echo "❌ Failed with exit code $EXIT_CODE" | tee -a "$MAIN_LOG"
        fi
        
        echo "" | tee -a "$MAIN_LOG"
        echo "Waiting 5 seconds before starting next script..." | tee -a "$MAIN_LOG"
        sleep 5
    else
        echo "❌ Script not found: $script" | tee -a "$MAIN_LOG"
    fi
done

TOTAL_END=$(date +%s.%N)
TOTAL_TIME=$(echo "$TOTAL_END - $TOTAL_START" | bc)

echo "" | tee -a "$MAIN_LOG"
echo "==================================================" | tee -a "$MAIN_LOG"
echo "ALL SCRIPTS COMPLETED" | tee -a "$MAIN_LOG"
echo "==================================================" | tee -a "$MAIN_LOG"
echo "Total execution time: $TOTAL_TIME seconds" | tee -a "$MAIN_LOG"
echo "Main log saved to: $MAIN_LOG" | tee -a "$MAIN_LOG"
echo "" | tee -a "$MAIN_LOG"

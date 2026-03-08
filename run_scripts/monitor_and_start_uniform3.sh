#!/bin/bash

echo "Monitoring analyze_rare3_fixed.sh and will start uniform3 when it finishes..."
echo "=========================================================================="

# Target script to monitor
TARGET_SCRIPT="analyze_rare3_fixed.sh"
SCRIPT_TO_START="/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/run_scripts/analyze_uniform3_fixed.sh"
LOG_FILE="/home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/run_scripts/monitor_and_start_uniform3.log"

echo "Monitoring: $TARGET_SCRIPT"
echo "Will start: $(basename "$SCRIPT_TO_START")"
echo "Log file: $LOG_FILE"
echo ""

# Function to check if script is running
is_script_running() {
    pgrep -f "$TARGET_SCRIPT" > /dev/null
    return $?
}

# Function to start the next script
start_uniform3() {
    echo "$(date): $TARGET_SCRIPT finished. Starting $(basename "$SCRIPT_TO_START")..." | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"
    
    cd /home/cohent59/PROJECT_RUN_PATTERN/pattern_finder
    nohup bash "$SCRIPT_TO_START" > /home/cohent59/PROJECT_RUN_PATTERN/pattern_finder/run_scripts/uniform3_auto_output.log 2>&1 &
    
    UNIFORM_PID=$!
    echo "$(date): Started $(basename "$SCRIPT_TO_START") with PID: $UNIFORM_PID" | tee -a "$LOG_FILE"
    echo "$(date): Log file: uniform3_auto_output.log" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"
    echo "✅ uniform3 started automatically!" | tee -a "$LOG_FILE"
    
    exit 0
}

# Main monitoring loop
echo "$(date): Starting monitoring..." | tee -a "$LOG_FILE"

while true; do
    if is_script_running; then
        echo "$(date): $TARGET_SCRIPT is still running... (checking every 30 seconds)" | tee -a "$LOG_FILE"
        sleep 30
    else
        echo "$(date): $TARGET_SCRIPT is no longer running!" | tee -a "$LOG_FILE"
        
        # Wait a bit to make sure it's really finished
        sleep 10
        
        # Double check
        if ! is_script_running; then
            start_uniform3
        else
            echo "$(date): False alarm, script is still running..." | tee -a "$LOG_FILE"
        fi
    fi
done

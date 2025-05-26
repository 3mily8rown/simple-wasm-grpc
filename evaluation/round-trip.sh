#!/bin/bash

output_file="metrics_log.txt"
echo "Run,Timestamp,Metric" > "$output_file"

num_runs=10  # Set how many runs you want

for ((i=1; i<=num_runs; i++)); do
    timestamp=$(date +"%Y-%m-%dT%H:%M:%S")
    echo "Starting run $i at $timestamp"

    # Start server in background and capture its PID
    cmake --build --preset=run-server &
    server_pid=$!

    # Wait briefly or check server readiness (e.g., wait for open port)
    sleep 2

    # Run client and capture output
    output=$(cmake --build --preset=run-client 2>&1)

    # Kill server process cleanly after client finishes
    kill $server_pid
    wait $server_pid 2>/dev/null

    # Extract and log [METRIC] lines from client output
    metric_lines=$(echo "$output" | grep "\[METRIC\]")
    while IFS= read -r line; do
        metric_only=$(echo "$line" | sed 's/\[METRIC\] //')
        echo "$i,$timestamp,$metric_only" >> "$output_file"
    done <<< "$metric_lines"

    echo "Run $i complete."
done

echo "All runs complete. Metrics logged to $output_file"

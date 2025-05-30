#!/bin/bash

output_file="metrics/round_trip_log_colocated_docker.txt"
echo "Run,Timestamp,Metric" > "$output_file"

num_runs=10  # Set how many runs you want

for ((i=1; i<=num_runs; i++)); do
    timestamp=$(date +"%Y-%m-%dT%H:%M:%S")
    echo "Starting run $i at $timestamp"

    # Run client and capture output
    output=$(docker compose up --build wasm_rpc_host --abort-on-container-exit 2>&1)

    docker compose down --volumes --remove-orphans

    # Extract and log [METRIC] lines from client output
    metric_lines=$(echo "$output" | grep "\[METRICS\]")
    while IFS= read -r line; do
        metric_only=$(echo "$line" | sed 's/\[METRICS\] //')
        echo "$i,$timestamp,$metric_only" >> "$output_file"
    done <<< "$metric_lines"

    echo "Run $i complete."
done

echo "All runs complete. Metrics logged to $output_file"

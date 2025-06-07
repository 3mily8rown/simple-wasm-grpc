#!/bin/bash
TARGET="$1"
/usr/bin/time -v "$TARGET" 2>&1 | tee /tmp/time_output.txt

CONTAINER_NAME=$(hostname)

grep "Maximum resident set size" /tmp/time_output.txt | awk -v name="$CONTAINER_NAME" '{ print "[METRICS] " name " max_rss_kb = " $NF }'
grep "User time (seconds)"       /tmp/time_output.txt | awk -v name="$CONTAINER_NAME" '{ print "[METRICS] " name " user_time_s = " $NF }'
grep "System time (seconds)"     /tmp/time_output.txt | awk -v name="$CONTAINER_NAME" '{ print "[METRICS] " name " sys_time_s = " $NF }'

#!/bin/bash

# Experiment parameters
BLOCKSIZE=16
L1_SIZE=1024      # 1KB
L1_ASSOC=1
L2_SIZE=0         # No L2 cache
L2_ASSOC=0
PREF_M=4          # Number of blocks in each stream buffer
TRACE_FILE="../test/streams_trace.txt"

# Output files
OUTPUT_CSV="experiment5results.csv"

# PREF_N values to test (0 to 4)
declare -a PREF_N_values=("0" "1" "2" "3" "4")

# Write header to CSV file
echo "PREF_N,L1_miss_rate" > $OUTPUT_CSV

# Loop over PREF_N values
for PREF_N in "${PREF_N_values[@]}"
do
    # Run the simulator
    OUTPUT=$(./sim $BLOCKSIZE $L1_SIZE $L1_ASSOC $L2_SIZE $L2_ASSOC $PREF_N $PREF_M $TRACE_FILE)

    # Parse the output to get the L1 miss rate
    L1_MISS_RATE_LINE=$(echo "$OUTPUT" | grep "^e\. L1 miss rate:")
    L1_MISS_RATE=$(echo "$L1_MISS_RATE_LINE" | awk '{print $5}')

    # Write to CSV
    echo "$PREF_N,$L1_MISS_RATE" >> $OUTPUT_CSV

    # Optional: Print progress to console
    echo "Completed: PREF_N=$PREF_N, L1 miss rate=$L1_MISS_RATE"

done
#!/bin/bash

# Experiment parameters
BLOCKSIZE=32
TRACE_FILE="../test/traces/gcc_trace.txt"

# Output files
OUTPUT_CSV="experiment_graph3_results.csv"

# L1 cache sizes to test (1KB, 2KB, 4KB, 8KB)
declare -a L1_sizes=("1024" "2048" "4096" "8192")

# L1 associativities to test (direct-mapped, 2-way, 4-way, 8-way)
declare -a L1_associativities=("1" "2" "4" "8")

# L2 cache parameters (16KB, 8-way set-associative, same block size as L1 cache)
L2_SIZE=16384  # 16KB
L2_ASSOC=8

# Write header to CSV file
echo "log2(L1_SIZE),L1_SIZE (Bytes),Associativity,L1_miss_rate,L2_miss_rate" > $OUTPUT_CSV

# Loop over L1 cache sizes
for L1_SIZE in "${L1_sizes[@]}"
do
    # Compute log2(L1_SIZE)
    case $L1_SIZE in
        1024)
            LOG2_L1_SIZE=10
            ;;
        2048)
            LOG2_L1_SIZE=11
            ;;
        4096)
            LOG2_L1_SIZE=12
            ;;
        8192)
            LOG2_L1_SIZE=13
            ;;
        *)
            LOG2_L1_SIZE=0
            ;;
    esac

    # Loop over L1 associativities
    for L1_ASSOC in "${L1_associativities[@]}"
    do
        # Run the simulator
        OUTPUT=$(./sim $BLOCKSIZE $L1_SIZE $L1_ASSOC $L2_SIZE $L2_ASSOC 0 0 $TRACE_FILE)

        # Parse the output to get L1 miss rate
        L1_MISS_RATE_LINE=$(echo "$OUTPUT" | grep "^e\. L1 miss rate")
        L1_MISS_RATE=$(echo "$L1_MISS_RATE_LINE" | awk '{print $5}')

        # Parse the output to get L2 miss rate
        L2_MISS_RATE_LINE=$(echo "$OUTPUT" | grep "^n\. L2 miss rate")
        L2_MISS_RATE=$(echo "$L2_MISS_RATE_LINE" | awk '{print $5}')

        # Write to CSV
        echo "$LOG2_L1_SIZE,$L1_SIZE,$L1_ASSOC,$L1_MISS_RATE,$L2_MISS_RATE" >> $OUTPUT_CSV

        # Write to TXT
        echo "log2(L1_SIZE): $LOG2_L1_SIZE, L1_SIZE: $L1_SIZE Bytes, Associativity: $L1_ASSOC, L1 miss rate: $L1_MISS_RATE, L2 miss rate: $L2_MISS_RATE" >> $OUTPUT_TXT

        # Optional: Print progress to console
        echo "Completed: L1_SIZE=$L1_SIZE Bytes, Associativity=$L1_ASSOC, L1 miss rate=$L1_MISS_RATE, L2 miss rate=$L2_MISS_RATE"

    done
done
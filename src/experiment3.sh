#!/bin/bash

# Experiment parameters
TRACE_FILE="../test/traces/gcc_trace.txt"

# Output files
OUTPUT_CSV="experiment_graph4_results.csv"
OUTPUT_TXT="experiment_graph4_results.txt"

# Associativity
ASSOC=4

# Block sizes to test (BLOCKSIZE = 16, 32, 64, 128)
declare -a block_sizes=("16" "32" "64" "128")

# Cache sizes to test (SIZE = 1KB, 2KB, 4KB, 8KB, 16KB, 32KB)
declare -a cache_sizes=("1024" "2048" "4096" "8192" "16384" "32768")

# Write header to CSV file
echo "log2(BLOCKSIZE),BLOCKSIZE (Bytes),L1_SIZE (Bytes),L1_miss_rate" > $OUTPUT_CSV

# Loop over cache sizes
for L1_SIZE in "${cache_sizes[@]}"
do
    # Loop over block sizes
    for BLOCKSIZE in "${block_sizes[@]}"
    do
        # Compute log2(BLOCKSIZE)
        case $BLOCKSIZE in
            16)
                LOG2_BLOCKSIZE=4
                ;;
            32)
                LOG2_BLOCKSIZE=5
                ;;
            64)
                LOG2_BLOCKSIZE=6
                ;;
            128)
                LOG2_BLOCKSIZE=7
                ;;
            *)
                LOG2_BLOCKSIZE=0
                ;;
        esac

        # Run the simulator
        OUTPUT=$(./sim $BLOCKSIZE $L1_SIZE $ASSOC 0 0 0 0 $TRACE_FILE)

        # Parse the output to get L1 miss rate
        MISS_RATE_LINE=$(echo "$OUTPUT" | grep "^e\. L1 miss rate")
        MISS_RATE=$(echo "$MISS_RATE_LINE" | awk '{print $5}')

        # Write to CSV
        echo "$LOG2_BLOCKSIZE,$BLOCKSIZE,$L1_SIZE,$MISS_RATE" >> $OUTPUT_CSV

        # Write to TXT
        echo "log2(BLOCKSIZE): $LOG2_BLOCKSIZE, BLOCKSIZE: $BLOCKSIZE Bytes, L1_SIZE: $L1_SIZE Bytes, L1 miss rate: $MISS_RATE" >> $OUTPUT_TXT

        # Optional: Print progress to console
        echo "Completed: BLOCKSIZE=$BLOCKSIZE Bytes, L1_SIZE=$L1_SIZE Bytes, L1 miss rate=$MISS_RATE"
    done
done
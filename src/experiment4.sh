#!/bin/bash

# Experiment parameters
BLOCKSIZE_L1=32
ASSOC_L1=4
BLOCKSIZE_L2=32
ASSOC_L2=8
TRACE_FILE="../test/traces/gcc_trace.txt"

# Output files
OUTPUT_CSV="experiment_graph5_results.csv"

# Define cache sizes
# L1 cache sizes (1KB, 2KB, 4KB, 8KB)
declare -a L1_sizes=("1024" "2048" "4096" "8192")
# L2 cache sizes (16KB, 32KB, 64KB)
declare -a L2_sizes=("16384" "32768" "65536")

# Write header to CSV file
echo "log2(L1_SIZE),L1_SIZE (Bytes),L2_SIZE (Bytes),L1_miss_rate,L2_miss_rate" > $OUTPUT_CSV

# Loop over L2 cache sizes
for L2_SIZE in "${L2_sizes[@]}"
do
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

        # Run the simulator
        OUTPUT=$(./sim $BLOCKSIZE_L1 $L1_SIZE $ASSOC_L1 $L2_SIZE $ASSOC_L2 0 0 $TRACE_FILE)

        # Parse the output to get miss rates
        L1_MISS_RATE_LINE=$(echo "$OUTPUT" | grep "^e\. L1 miss rate")
        L1_MISS_RATE=$(echo "$L1_MISS_RATE_LINE" | awk '{print $5}')
        L2_MISS_RATE_LINE=$(echo "$OUTPUT" | grep "^n\. L2 miss rate")
        L2_MISS_RATE=$(echo "$L2_MISS_RATE_LINE" | awk '{print $5}')

        # Remove any commas in the miss rates (if formatted with commas)
        L1_MISS_RATE=${L1_MISS_RATE//,/}
        L2_MISS_RATE=${L2_MISS_RATE//,/}

        # Write to CSV
        echo "$LOG2_L1_SIZE,$L1_SIZE,$L2_SIZE,$L1_MISS_RATE,$L2_MISS_RATE" >> $OUTPUT_CSV

        # Write to TXT
        echo "log2(L1_SIZE): $LOG2_L1_SIZE, L1_SIZE: $L1_SIZE Bytes, L2_SIZE: $L2_SIZE Bytes, L1 miss rate: $L1_MISS_RATE, L2 miss rate: $L2_MISS_RATE" >> $OUTPUT_TXT

        # Optional: Print progress to console
        echo "Completed: L1_SIZE=$L1_SIZE Bytes, L2_SIZE=$L2_SIZE Bytes, L1 miss rate=$L1_MISS_RATE, L2 miss rate=$L2_MISS_RATE"

    done
done
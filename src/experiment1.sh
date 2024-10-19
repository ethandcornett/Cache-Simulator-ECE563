#!/bin/bash

# Experiment parameters
BLOCKSIZE=32
TRACE_FILE="../test/traces/gcc_trace.txt"

# Output files
OUTPUT_CSV="experiment_results.csv"

# Write header to CSV file
echo "log2(L1_SIZE),L1_SIZE (Bytes),Associativity,L1_miss_rate" > $OUTPUT_CSV

# Associativities to test
declare -a associativities=("1" "2" "4" "8" "FULLY")

# Loop over cache sizes (from 1KB to 1MB in powers of two)
for ((exp=10; exp<=20; exp++))
do
    L1_SIZE=$((2**exp))  # L1_SIZE in bytes
    LOG2_L1_SIZE=$exp

    for ASSOC in "${associativities[@]}"
    do
        if [ "$ASSOC" == "FULLY" ]; then
            # Fully-associative cache: associativity equals number of blocks
            L1_ASSOC=$((L1_SIZE / BLOCKSIZE))
        else
            L1_ASSOC=$ASSOC
        fi

        # Run the simulator
        OUTPUT=$(./sim $BLOCKSIZE $L1_SIZE $L1_ASSOC 0 0 0 0 $TRACE_FILE)

        # Parse the output to get L1 miss rate
        MISS_RATE_LINE=$(echo "$OUTPUT" | grep "^e\. L1 miss rate")
        MISS_RATE=$(echo "$MISS_RATE_LINE" | awk '{print $5}')

        # Write to CSV
        echo "$LOG2_L1_SIZE,$L1_SIZE,$ASSOC,$MISS_RATE" >> $OUTPUT_CSV

        # Write to TXT
        echo "log2(L1_SIZE): $LOG2_L1_SIZE, L1_SIZE: $L1_SIZE Bytes, Associativity: $ASSOC, L1 miss rate: $MISS_RATE" >> $OUTPUT_TXT

        # Optional: Print progress to console
        echo "Completed: L1_SIZE=$L1_SIZE Bytes, Associativity=$ASSOC, L1 miss rate=$MISS_RATE"

    done
done
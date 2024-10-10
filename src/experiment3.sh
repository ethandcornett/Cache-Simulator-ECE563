#!/bin/bash

# Experiment parameters
BLOCKSIZE_L1=32
ASSOC_L1=4
BLOCKSIZE_L2=32
ASSOC_L2=8
TRACE_FILE="../test/traces/gcc_trace.txt"

# Output files
OUTPUT_CSV="experiment_graph5_results.csv"
OUTPUT_TXT="experiment_graph5_results.txt"

# Define cache sizes
# L1 cache sizes (1KB, 2KB, 4KB, 8KB)
declare -a L1_sizes=("1024" "2048" "4096" "8192")
# L2 cache sizes (16KB, 32KB, 64KB)
declare -a L2_sizes=("16384" "32768" "65536")

# Access times (in cycles)
# These are assumed values; adjust them according to your system specifications
L1_Hit_Time=1
L2_Hit_Time=10
Memory_Access_Time=100

# Write header to CSV file
echo "log2(L1_SIZE),L1_SIZE (Bytes),L2_SIZE (Bytes),AAT" > $OUTPUT_CSV

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

        # Parse the output to get miss rates and other necessary data
        L1_MISS_RATE_LINE=$(echo "$OUTPUT" | grep "^e\. L1 miss rate")
        L1_MISS_RATE=$(echo "$L1_MISS_RATE_LINE" | awk '{print $5}')
        L2_MISS_RATE_LINE=$(echo "$OUTPUT" | grep "^n\. L2 miss rate")
        L2_MISS_RATE=$(echo "$L2_MISS_RATE_LINE" | awk '{print $5}')

        # Remove any commas in the miss rates (if formatted with commas)
        L1_MISS_RATE=${L1_MISS_RATE//,/}
        L2_MISS_RATE=${L2_MISS_RATE//,/}

        # Compute AAT
        # AAT = L1_Hit_Time + L1_Miss_Rate * (L2_Hit_Time + L2_Miss_Rate * Memory_Access_Time)
        AAT=$(echo "$L1_Hit_Time + $L1_MISS_RATE * ($L2_Hit_Time + $L2_MISS_RATE * $Memory_Access_Time)" | bc -l)

        # Format AAT to 4 decimal places
        AAT=$(printf "%.4f" $AAT)

        # Write to CSV
        echo "$LOG2_L1_SIZE,$L1_SIZE,$L2_SIZE,$AAT" >> $OUTPUT_CSV

        # Write to TXT
        echo "log2(L1_SIZE): $LOG2_L1_SIZE, L1_SIZE: $L1_SIZE Bytes, L2_SIZE: $L2_SIZE Bytes, AAT: $AAT cycles" >> $OUTPUT_TXT

        # Optional: Print progress to console
        echo "Completed: L1_SIZE=$L1_SIZE Bytes, L2_SIZE=$L2_SIZE Bytes, AAT=$AAT cycles"

    done
done
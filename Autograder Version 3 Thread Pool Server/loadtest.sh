#!/bin/bash

# Check for the correct number of arguments
if [ "$#" -ne 4 ]; then
  echo "Usage: ./loadtest.sh <numClients> <loopNum> <sleepTimeSeconds> <timeout-seconds>"
  exit 1
fi

# Parse arguments
numClients=$1
loopNum=$2
sleepTime=$3
timeout=$4

port=8080

# Create a directory to store the experiment data
EXPERIMENT_DIR="experiment_data_M${numClients}"
mkdir -p $EXPERIMENT_DIR

# Function to capture vmstat and ps snapshots
capture_snapshots() {
    local snapshot_file_cpu="${EXPERIMENT_DIR}/cpu_snapshot_M${numClients}_$1.txt"
    local snapshot_file_nlwp="${EXPERIMENT_DIR}/nlwp_snapshot_M${numClients}_$1.txt"

    # Capture vmstat output every 10 seconds
    (vmstat 1 $((loopNum * sleepTime )) > $snapshot_file_cpu) &

    # Capture ps output for NLWP every 5 seconds
    (while [ $SECONDS -lt $((loopNum * sleepTime)) ]; do
        ps -eLf | grep "./server $port" | head -1 | awk '{print $6}' >> "$snapshot_file_nlwp"
        sleep 5
    done) &
}

# Create a directory to store the loadtest-output data
OUTPUT_DIR="output_data_M${numClients}"
mkdir -p $OUTPUT_DIR

for (( i=1 ; i<=$numClients ; i++ )); 
do	
	echo "Launching client $i"
	capture_snapshots $i
	filename="${OUTPUT_DIR}/output_$i.txt" 
	./submit 127.0.0.1:$port code.cpp $loopNum $sleepTime $timeout > "$filename" & 
done
echo "All client launched"

wait

# Initialize a variable to store the total idle_sum
total_idle_sum=0

# Initialize a variable to store the total number of records
total_nr=0

# Initialize a variable to count the number of files
file_count=0

# Iterate over the CPU snapshot files
for file in ${EXPERIMENT_DIR}/cpu_snapshot_M${numClients}_*.txt; do
    # Calculate the idle_sum for each file
    idle_sum=$(awk 'NR > 2 { idle_sum += $15 } END { print idle_sum }' "$file")
    
    # Add the idle_sum to the total
    total_idle_sum=$((total_idle_sum + idle_sum))
    
    # Calculate the number of records (NR) for each file
    nr=$(awk 'END { print NR }' "$file")
    
    # Add the NR to the total
    total_nr=$((total_nr + nr))
    
    # Increment the file count
    file_count=$((file_count + 1))
done

# Calculate the overall average CPU utilization
average_cpu_utilization=$(echo "scale=2; 100 - ($total_idle_sum / ($total_nr - $file_count*2))" | bc)



# Initialize a variable to store the total NLWP sum
total_nlwp_sum=0

# Initialize a variable to store the total number of records
total_nr=0

# Iterate over the NLWP snapshot files
for file in ${EXPERIMENT_DIR}/nlwp_snapshot_M${numClients}_*.txt; do
    # Calculate the NLWP sum for each file
    nlwp_sum=$(awk '{s+=$1} END {print s}' "$file")
    
    # Add the NLWP sum to the total
    total_nlwp_sum=$((total_nlwp_sum + nlwp_sum))
    
    # Calculate the number of records (NR) for each file
    nr=$(awk 'END { print NR }' "$file")
    
    # Add the NR to the total
    total_nr=$((total_nr + nr))
done

# Calculate the overall average NLWP
average_nlwp=$(echo "scale=2; $total_nlwp_sum / $total_nr" | bc)

overall_request_rate_sent=0
goodput=0
overall_timeout_rate=0
overall_throughput=0
avgResponseTime=0
totalsuccessfulResponseCnt=0

for ((i = 1; i <= numClients; i++)); do
    filename="${OUTPUT_DIR}/output_$i.txt" 
    total_time_i=$(grep "Total Loop Time: " "$filename"  | cut -d ' ' -f 4)
    
    # Request Rate Sent (requests per second)
    overall_request_rate_sent=$(echo "scale=2; $overall_request_rate_sent + $loopNum * 1000 / $total_time_i" | bc)
    
    # Goodput (successful requests per second)
    successfulResponseCnt_i=$(grep "Successful Response Count: " "$filename"  | cut -d ' ' -f 4)
    totalsuccessfulResponseCnt=$(echo "scale=2; $totalsuccessfulResponseCnt + $successfulResponseCnt_i" | bc)
    goodput=$(echo "scale=2; $goodput + $successfulResponseCnt_i * 1000 / $total_time_i" | bc)
    
    # Timeout Rate 
    timeout_responses_Cnt_i=$(grep "Timeout Count: " "$filename" | cut -d ' ' -f 3)
    overall_timeout_rate=$(echo "scale=2; $overall_timeout_rate + $timeout_responses_Cnt_i * 1000 / $total_time_i" | bc)
    
    avgResponseTime_i=$(grep "Average Response Time: " "$filename"  | cut -d ' ' -f 4)
    overall_throughput=$(echo "scale=2; $overall_throughput + 1000 / $avgResponseTime_i" | bc)
    avgResponseTime=$(echo "scale=2; $avgResponseTime + $successfulResponseCnt_i*$avgResponseTime_i" | bc)
done

avgResponseTime=$(echo "scale=2; $avgResponseTime / $totalsuccessfulResponseCnt" | bc)


# Error Rate 
overall_error_rate=$(echo "scale=2; $overall_request_rate_sent - ($goodput + $overall_timeout_rate)" | bc)

#calculating queue length 
file_path="queue_length.txt"

# Check if the file exists
if [ ! -f "$file_path" ]; then
    echo "File not found: $file_path"
    exit 1
fi

# Use awk to sum all values in the file
total=0
records=0
while read -r value; do
    total=$((total + value))
    records=$((records + 1))
done < "$file_path"

avg_queue_length=$((total/records))
> queue_length.txt

#save data to textfile
echo "$numClients $overall_request_rate_sent" >> request_rate_sent_data.txt
echo "$numClients $goodput" >> goodput_data.txt
echo "$numClients $overall_timeout_rate" >> timeout_rate_data.txt
echo "$numClients $overall_error_rate" >> error_rate_data.txt
echo "$numClients $average_cpu_utilization" >> cpu_utilization_data.txt
echo "$numClients $average_nlwp" >> active_threads_data.txt
echo "$numClients $overall_throughput" >> throughput_data.txt
echo "$numClients $avgResponseTime" >> response_time_data.txt
echo "$numClients $avg_queue_length" >> avg_queue_length.txt


#! /usr/bin/bash 

if [ $# -ne 4 ]; then
	echo "Usage $0 <numClients> <loopNum> <sleepTimeSeconds> <timeout-seconds>"
	exit 1
fi
source_file="$1"
arg2="$2"
arg3="$3"
arg4="$4" 

port=8080
M=$1
x=1
total_throughput=0
sum_N=0
sum_NR=0
numberofResponses=0
avgreqrate=0
avgsuccessfulrate=0
avgtimeoutrate=0
avgerrorrate=0
total_bussy=0
total_threads=0
rm -r loadtest-output
mkdir loadtest-output

numClients=$1
loopNum=$2
sleepTime=$3
# Create a directory to store the experiment data
EXPERIMENT_DIR="experiment_data_M${numClients}"
mkdir -p $EXPERIMENT_DIR


# Function to capture vmstat and ps snapshots
capture_snapshots() {
    local snapshot_file_cpu="${EXPERIMENT_DIR}/cpu_snapshot_M${numClients}_$1.txt"
    local snapshot_file_nlwp="${EXPERIMENT_DIR}/nlwp_snapshot_M${numClients}_$1.txt"

    # Capture vmstat output every 10 seconds
    (vmstat 1 $((loopNum * sleepTime)) > $snapshot_file_cpu) &

    # Capture ps output for NLWP every 10 seconds
    ( while [ $SECONDS -lt $((loopNum * sleepTime)) ]; do
       	ps -eLf | grep "./gradingserver $port" | head -1 | awk '{print $6}' >> "$snapshot_file_nlwp"
        sleep 5
    done) &
}

for (( i=1 ; i<=$M ; i++ )); 
do	
	echo "Launching client $i"
	capture_snapshots $i
	filename="./loadtest-output/output$i.txt"  
	./submit 127.0.0.1:$port code.cpp $arg2 $arg3 $arg4 > "$filename" & 
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
for file in ${EXPERIMENT_DIR}/cpu_snapshot_M${M}_*.txt; do
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

for (( i=1 ; i<=$M ; i++ )); 
do
	filename="./loadtest-output/output$i.txt"  
	avg_res=$(grep "Average Response Time:" "$filename" | cut -d ' ' -f 4)
	N_i=$(grep "Number of Successful Responses:" "$filename" | cut -d ' ' -f 5)
	avgreqrate_i=$(grep "Request sent rate:" "$filename" | cut -d ' ' -f 4)
  	avgsuccessfulrate_i=$(grep "Successful request rate:" "$filename" | cut -d ' ' -f 4)
  	avgtimeoutrate_i=$(grep "Timeout rate:" "$filename" | cut -d ' ' -f 3)
  	avgerrorrate_i=$(grep "Error rate:" "$filename" | cut -d ' ' -f 3)
  	numberofResponses=$(echo "$numberofResponses+$N_i" | bc -l)
  	avgreqrate=$(echo "$avgreqrate+$avgreqrate_i" | bc -l)
  	avgsuccessfulrate=$(echo "$avgsuccessfulrate+$avgsuccessfulrate_i" | bc -l)
  	avgtimeoutrate=$(echo "$avgtimeoutrate+$avgtimeoutrate_i" | bc -l)
  	avgerrorrate=$(echo "$avgerrorrate+$avgerrorrate_i" | bc -l)
	
	#Calculate the throughput and total throughput
	throughput=$(echo $x*1000/$avg_res | bc -l)
	total_throughput=$(echo $total_throughput+$throughput | bc -l)
	
	#Calculate the sum of N_i*R_i i.e. number_of_successful*average_response_time 
	sum_NR=$(echo $sum_NR+$avg_res*$N_i | bc -l)
	sum_N=$(echo $N_i+$sum_N | bc -l)	
done

total_avg_response_time=$(echo $sum_NR/$sum_N | bc -l)
#echo "Overall throughput: $total_throughput requests/sec" 
#echo "Average response time: $total_avg_response_time"

echo "$M $total_throughput" >> throughput_data.txt
echo "$M $total_avg_response_time" >> response_time_data.txt
echo "$M $avgreqrate" >> request_rate.txt
echo "$M $avgsuccessfulrate" >> goodput_data.txt
echo "$M $avgtimeoutrate" >> timeout_rate.txt
echo "$M $avgerrorrate" >> error_rate.txt
echo "$M $average_cpu_utilization" >> utilization_result.txt
echo "$M $average_nlwp" >> no_of_threads.txt

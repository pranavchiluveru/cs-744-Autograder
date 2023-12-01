#!/bin/bash

# Check for the correct number of arguments
if [ "$#" -ne 3 ]; then
  echo "Usage: ./loadtest.sh <numClients> <loopNum> <sleepTimeSeconds>"
  exit 1
fi

# Parse arguments
numClients=$1
loopNum=$2
sleepTime=$3
port=8000

x=1
total_throughput=0
sum_N=0
sum_NR=0
numberofResponses=0


rm -r loadtest-output
mkdir loadtest-output


for (( i=1 ; i<=$numClients ; i++ )); 
do	
	echo "Launching client $i"
	filename="./loadtest-output/output$i.txt"  
	./submit 127.0.0.1:$port code.cpp $loopNum $sleepTime > "$filename" & 
done
echo "All client launched"

wait

for (( i=1 ; i<=$numClients; i++ )); 
do
	filename="./loadtest-output/output$i.txt"  
	avg_res=$(grep "Average Response Time:" "$filename" | cut -d ' ' -f 4)
	N_i=$(grep "Number of Successful Responses:" "$filename" | cut -d ' ' -f 5)
	
	#Calculate the throughput and total throughput
	throughput=$(grep "Throughput:" "$filename" | cut -d ' ' -f 2)
	total_throughput=$(echo $total_throughput + $throughput | bc -l)
	
	#Calculate the sum of N_i*R_i i.e. number_of_successful*average_response_time 
	sum_NR=$(echo $sum_NR+$avg_res*$N_i | bc -l)
	sum_N=$(echo $N_i+$sum_N | bc -l)	
done

total_avg_response_time=$(echo $sum_NR/$sum_N| bc -l)
#echo "Overall throughput: $total_throughput requests/sec" 
#echo "Average response time: $total_avg_response_time"

echo "$numClients $total_throughput" >> throughput_data.txt
echo "$numClients $total_avg_response_time" >> response_time_data.txt















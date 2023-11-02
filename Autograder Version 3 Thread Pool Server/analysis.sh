#!/bin/bash

# Different number of Client
numClient='
1
2
5
10
15
20
25
30
32
35
'

PROGRAM='loadtest.sh';
loopNum=3
sleepTime=3
timeout=100

# Run the analysis for different sizes of client
for i in ${numClient}; do
    ./$PROGRAM $i $loopNum $sleepTime $timeout
done

wait

echo Analysing and Generating PNG!!

# Plot the results

cat request_rate_sent_data.txt| graph -T png --bitmap-size "1400x1400" -g 3 -L "Request rate sent vs No. of clients" -X "No. of Clients" -Y "Request rate sent" -r 0.25> ./plot1.png
cat goodput_data.txt| graph -T png --bitmap-size "1400x1400" -g 3 -L "goodput vs No. of clients" -X "No. of Clients" -Y "goodput " -r 0.25> ./plot2.png
cat timeout_rate_data.txt| graph -T png --bitmap-size "1400x1400" -g 3 -L "timeout rate vs No. of clients" -X "No. of Clients" -Y "timeout rate" -r 0.25> ./plot3.png
cat cpu_utilization_data.txt| graph -T png --bitmap-size "1400x1400" -g 3 -L "cpu utilization vs No. of clients" -X "No. of Clients" -Y "cpu utilization %" -r 0.25> ./plot4.png
cat error_rate_data.txt| graph -T png --bitmap-size "1400x1400" -g 3 -L "error rate vs No. of clients" -X "No. of Clients" -Y "error rate" -r 0.25> ./plot5.png
cat active_threads_data.txt| graph -T png --bitmap-size "1400x1400" -g 3 -L "Avg. no. of active threads vs No. of clients" -X "No. of Clients" -Y "Avg. no. of active threads" -r 0.25> ./plot6.png
cat throughput_data.txt| graph -T png --bitmap-size "1400x1400" -g 3 -L "Overall Throughtput vs No. of clients" -X "No. of Clients" -Y "Throughput" -r 0.25> ./plot7.png
cat response_time_data.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Avg. Response Time vs No. of clients " -X "No. of Clients" -Y "Avg. Response Time(in ms)" -r 0.25> ./plot8.png
cat avg_queue_length.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Avg. No of Requests in Queue vs No. of clients " -X "No. of Clients" -Y "Avg.  No. of Requests in Queue" -r 0.25> ./plot9.png

echo "Done"



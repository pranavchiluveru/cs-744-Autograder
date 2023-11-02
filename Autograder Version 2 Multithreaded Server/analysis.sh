#!/bin/bash
# Different number of Client
numClient='
1
5
10
15
20
25
30
35
'

PROGRAM='loadtest1.sh';
loopNum=3
sleepTime=1
timeout=50

# Run the analysis for different sizes of client
for i in ${numClient}; do
    ./$PROGRAM $i $loopNum $sleepTime $timeout
done

# Plot the results
echo Analysing and Generating PNG!!
cat throughput_data.txt| graph -T png --bitmap-size "1400x1400" -g 3 -L "Overall Throughtput vs No. of clients" -X "No. of Clients" -Y "Throughput" -r 0.25> ./plot1.png

cat response_time_data.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Avg. Response Time vs No. of clients " -X "No. of Clients" -Y "Avg. Response Time(in ms)" -r 0.25> ./plot2.png

cat request_rate.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Request Rate vs No. of clients " -X "No. of Clients" -Y "Request Rate" -r 0.25> ./plot3.png

cat goodput_data.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Successful request rate vs No. of clients " -X "No. of Clients" -Y "Successful request rate" -r 0.25> ./plot4.png

cat timeout_rate.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Timeout rate vs No. of clients " -X "No. of Clients" -Y "Timeout rate" -r 0.25> ./plot5.png

cat error_rate.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Error rate vs No. of clients " -X "No. of Clients" -Y "Error rate" -r 0.25> ./plot6.png

cat utilization_result.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "CPU Utilization vs No. of clients " -X "No. of Clients" -Y "CPU utilization" -r 0.25> ./plot7.png

cat no_of_threads.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Average number of active threads vs No. of clients " -X "No. of Clients" -Y "Average number of active threads" -r 0.25> ./plot8.png


echo "Done"



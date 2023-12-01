#!/bin/bash
echo Analysing and Generating PNG!!

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

PROGRAM='loadtest.sh';
loopNum=3
sleepTime=1

# Run the analysis for different sizes of client
for i in ${numClient}; do
    ./$PROGRAM $i $loopNum $sleepTime
done

wait

# Plot the results
cat throughput_data.txt| graph -T png --bitmap-size "1400x1400" -g 3 -L "Overall Throughtput vs No. of clients" -X "No. of Clients" -Y "Throughput(req/sec)" -r 0.25> ./plot1.png


cat response_time_data.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Avg. Response Time vs No. of clients " -X "No. of Clients" -Y "Avg. Response Time(in ms)" -r 0.25> ./plot2.png

echo "Done"



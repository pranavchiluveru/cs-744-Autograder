#!/bin/bash

# Function to submit a new grading request and extract the request ID
submit_new() {
    local result_msg
    result_msg=$(./submit new "$1" "$2")
    echo "$result_msg"
}

# Function to check the status until "processing is done"
check_status() {
    local server_ip_port=$1
    local request_id=$2
    local polling_interval=$3
    
    local start_time
    start_time=$(date +%s%N)  # Get the start time in nanoseconds

    # Initial submission
    response=$(./submit status "$server_ip_port" "$request_id")
    echo "$response"

    # Polling loop
    while [[ "$response" != *"Processing is done."* ]]; do
        sleep "$polling_interval"
        response=$(./submit status "$server_ip_port" "$request_id")
        echo "$response"
    done

    local end_time
    end_time=$(date +%s%N)  # Get the end time in nanoseconds

    local response_time
    response_time=$(( (end_time - start_time) / 1000000 ))  # Convert nanoseconds to milliseconds

    echo "Grading done for Request ID: $request_id"
    echo "Response Time: ${response_time}ms" >> response_time.txt
    echo "Response Time for Request ID $request_id: ${response_time}ms"  # Display on the terminal
}

# Example usage
server_ip_port="127.0.0.1:8080"  # Replace with your actual server IP and port
source_code_files=("example_code1.cpp" "example_code2.cpp" "example_code3.cpp" "example_code4.cpp")  # Replace with your actual source code files
num_requests=${#source_code_files[@]}

# Loop through each source code file, submit a new grading request, and check status
for ((i=0; i<num_requests; i++)); do
    source_code_file=${source_code_files[i]}

    # Submit a new grading request and capture the result message
    request_msg=$(submit_new "$server_ip_port" "$source_code_file")
    echo "$request_msg"
    
    # Extract request ID from the result message
    request_id=$(echo "$request_msg" | grep -oP 'Your grading request ID \K[0-9]+')
    
    # Check status with polling (interval: 1 second)
    polling_interval=1
    check_status "$server_ip_port" "$request_id" "$polling_interval"
done


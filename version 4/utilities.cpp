#include "utilities.h"
#include <cstdlib> 
#include <ctime>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream> 
#include <mutex>
#include <sys/socket.h>
#include <sys/types.h>
#include <sstream>

using namespace std;

string generateUniqueID() {
    int randomNum = rand() % 10000;
    time_t t;
    struct tm* now;
    char timestamp[20];
    time(&t);
    now = localtime(&t);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", now);
    return string(timestamp) + to_string(randomNum)  ;
}

std::mutex fileMutex; // Mutex to synchronize file access

// Function to append a log entry to the file
void appendLogEntry(const string& requestID, const string& output, const string& filename) {
	
    lock_guard<std::mutex> lock(fileMutex); // Lock to ensure exclusive access
    ofstream logFile("gradingLog.txt", ios::app | ios::out);

    if (!logFile.is_open()) {
        cerr << "Error opening the log file!" << endl;
        return;
    }

    logFile << requestID << ":" << endl << output << endl;

    ifstream sourceFile;
    // Read content from the source file and append it to the destination file
    if (!filename.empty()) {
        sourceFile.open(filename);
        string line;
        while (getline(sourceFile, line)) {
            logFile << line << endl;
        }
        sourceFile.close();
    }

    logFile << RECORD_END_MARKER << endl;
    logFile << endl;
    logFile.close();
}

// Function to search for a request ID in the log file
string searchLog(const std::string& filename, const std::string& requestId) {
    lock_guard<std::mutex> lock(fileMutex); // Lock to ensure exclusive access
    ifstream logFile(filename);

    if (!logFile.is_open()) {
        cerr << "Error opening the log file!" << std::endl;
        return "Error opening the log file!";
    }

    string line;
    stringstream result;
    string to_match = requestId + ":";

    while (getline(logFile, line)) {
        size_t found = line.find(to_match);
        if (found != string::npos) {
            // Append the line to the result stringstream until the end marker is found
            while (getline(logFile, line) && line != RECORD_END_MARKER) {
                result << line << endl;
            }

            break; // Stop searching after finding the corresponding output
        }
    }

    logFile.close();

    if (result.str().empty()) {
        return "Grading request " + requestId + " not found. Please check and resend your request ID or re-send your original grading request.";
    } else {
        return result.str();
    }
}


//Utility Function to receive a file of any size to the grading server
int recv_file(int sockfd,const char *file_path)
//Arguments: socket fd, file name (can include path) into which we will store the received file
{
    char buffer[BUFFER_SIZE]; //buffer into which we read  the received file chars
    bzero(buffer, BUFFER_SIZE); //initialize buffer to all NULLs
    FILE *file = fopen(file_path, "wb");  //Get a file descriptor for writing received data into file
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }

    //buffer for getting file size as bytes
    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    //first receive  file size bytes
    if (recv(sockfd, file_size_bytes, sizeof(file_size_bytes), 0) == -1)
    {
        perror("Error receiving file size");
        fclose(file);
        return -1;
    }
   
    int file_size;
    //copy bytes received into the file size integer variable
    memcpy(&file_size, file_size_bytes, sizeof(file_size_bytes));
    
    //some local printing for debugging
    printf("File size is: %d\n", file_size);
    
    //now start receiving file data
    size_t bytes_read = 0, total_bytes_read =0;;
    while (true)
    {
    	//read max BUFFER_SIZE amount of file data
        bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0);

        //total number of bytes read so far
        total_bytes_read += bytes_read;

        if (bytes_read <= 0)
        {
            perror("Error receiving file data");
            fclose(file);
            return -1;
        }

	//write the buffer to the file
        fwrite(buffer, 1, bytes_read, file);

	// reset buffer
        bzero(buffer, BUFFER_SIZE);
        
       //break out of the reading loop if read file_size number of bytes
        if (total_bytes_read >= file_size)
            break;
    }
    fclose(file);
    return 0;
}



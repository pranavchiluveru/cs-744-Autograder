#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <pthread.h>
#include <unordered_map>


using namespace std; 

const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;
const string RECORD_END_MARKER = "###END###";

// Mutex for protecting the hash map
extern pthread_mutex_t mapMutex;

// Structure to hold grading request information
struct GradingRequestInfo {
    string status;
    int queuePos;
    string fileName;
};

// Hash map to store information about grading requests
extern unordered_map<string, GradingRequestInfo> gradingRequestMap;

string generateUniqueID();
int recv_file(int sockfd, const char *file_path);
string searchLog(const string& filename, const string& requestID);
void appendLogEntry(const string& requestID, const string& output, const string& filename);

#endif


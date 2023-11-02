#include "utilities.h"
#include <cstdlib> 
#include <ctime>
#include <cstring>
#include <string>
#include <iostream> 

using namespace std;

bool isOutputCorrect(const string& programOutput) {
    string expectedOutput = "1 2 3 4 5 6 7 8 9 10";
    return programOutput == expectedOutput;
}


string generateUniqueFileName() {
    int randomNum = rand() % 10000;
    time_t t;
    struct tm* now;
    char timestamp[20];
    time(&t);
    now = localtime(&t);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", now);
    return string(timestamp) + to_string(randomNum)  ;
}



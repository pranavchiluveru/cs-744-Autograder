#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>

const int BUFFER_SIZE = 1024;

// Create a structure to hold client socket and any other necessary data
struct ThreadArgs {
    int Socket;
};

bool isOutputCorrect(const std::string& programOutput);
std::string generateUniqueFileName();

#endif


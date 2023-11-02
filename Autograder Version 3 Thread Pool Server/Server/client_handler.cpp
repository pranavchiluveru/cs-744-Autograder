#include "client_handler.h"
#include "compiler.h"
#include "utilities.h"
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
using namespace std;

void* handleClient(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int clientSocket = args->Socket;

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    
    // Receive source code from the client
    ssize_t sourceBytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (sourceBytesRead == -1) {
        cerr << "Error receiving data." << endl;
        close(clientSocket);
        delete args;
        //pthread_exit(NULL);
    }
    
    compileAndRunCode(buffer,sourceBytesRead,arg);
    return NULL;
    
}


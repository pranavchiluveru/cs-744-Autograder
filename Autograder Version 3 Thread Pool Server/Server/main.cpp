#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <queue> 
#include <fstream>
#include "utilities.h"
#include "client_handler.h"
using namespace std;

queue<int> gradingRequests;
pthread_mutex_t queueMutex;
pthread_cond_t queueReady;
const int maxQueueSize = 50;

void* threadWorker(void* arg) {
    while (true) {
        pthread_mutex_lock(&queueMutex);
        while (gradingRequests.empty()) {
            pthread_cond_wait(&queueReady, &queueMutex);
        }
        int clientSocket = gradingRequests.front();
        gradingRequests.pop();
        
        pthread_mutex_unlock(&queueMutex);

        ThreadArgs* args = new ThreadArgs;
        args->Socket = clientSocket;

        handleClient(args);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <port> <thread_pool_size>" << endl;
        return 1;
    }

    int threadPoolSize = atoi(argv[2]);
    if (threadPoolSize <= 0 ) {
        cerr << "Thread pool size must be greater than 0"<< endl;
        return 1;
    }

    pthread_t threads[threadPoolSize];
    pthread_mutex_init(&queueMutex, NULL);
    pthread_cond_init(&queueReady, NULL);

    // Create thread pool
    for (int i = 0; i < threadPoolSize; i++) {
        if (pthread_create(&threads[i], NULL, threadWorker, NULL) != 0) {
            cerr << "Failed to create a worker thread." << endl;
            return 1;
        }
    }

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    // Create socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cerr << "Error creating socket." << endl;
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[1]));
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Error binding socket." << endl;
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 100) == -1) {
        cerr << "Error listening for connections." << endl;
        return 1;
    }
    

    cout << "Server listening on port " << atoi(argv[1]) << "..." << endl;

    while (true) {
        // Accept incoming connection
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == -1) {
            cerr << "Error accepting connection." << endl;
            continue;
        }
        std::ofstream file("queue_length.txt", std::ios::out | std::ios::app);
         // Add the client socket to the grading queue
        pthread_mutex_lock(&queueMutex);
        if (gradingRequests.size() < maxQueueSize) {
        gradingRequests.push(clientSocket); 
        file<< gradingRequests.size() << std::endl;
        pthread_cond_signal(&queueReady);
        }
        pthread_mutex_unlock(&queueMutex);
    }
    close(serverSocket);

    return 0;
}


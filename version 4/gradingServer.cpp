#include <bits/stdc++.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <queue> 
#include <fstream>
#include "utilities.h"
#include "compiler.h"
using namespace std;

queue<string> gradingRequests;
pthread_mutex_t queueMutex;
pthread_cond_t queueReady;
const int maxQueueSize = 50;

pthread_mutex_t mapMutex = PTHREAD_MUTEX_INITIALIZER;
unordered_map<string, GradingRequestInfo> gradingRequestMap;

void* threadWorker(void* arg) {
    while (true) {
        pthread_mutex_lock(&queueMutex);
        while (gradingRequests.empty()) {
            pthread_cond_wait(&queueReady, &queueMutex);
        }
        string requestID  = gradingRequests.front();
        gradingRequests.pop();
        
        pthread_mutex_unlock(&queueMutex);
        
        pthread_mutex_lock(&mapMutex);
	gradingRequestMap[requestID].status = "Processing";
	
	string file_path = gradingRequestMap[requestID].fileName;
	
	pthread_mutex_unlock(&mapMutex);
        compileAndRunCode(file_path,requestID);
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
        
        char buffer[BUFFER_SIZE]; //buffer into which we read  the received file chars
    	bzero(buffer, BUFFER_SIZE);
    	
    	 // Add the client socket to the grading queue
        pthread_mutex_lock(&queueMutex);
    	
        if (gradingRequests.size() < maxQueueSize) {
        	//read client response (requestType)
	        ssize_t bytes_read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
	        if (bytes_read <= 0) {
		    cerr << "Error receiving requestType." << endl;
		    close(clientSocket);
	        }
	       
	        if(strcmp(buffer, "new") == 0){
	        	//generate requestID and push ID to queue. add map hash table with status accepted.
			string requestID = generateUniqueID();
			gradingRequests.push(requestID);
			 
			GradingRequestInfo info;
			info.status = "Queued";
			info.queuePos = gradingRequests.size();
			
			memset(buffer, 0,  BUFFER_SIZE);
			
			string sourceFileName = "code_" +requestID + ".cpp";
			// Receive source code from the client
			if (recv_file(clientSocket , sourceFileName.c_str()) != 0){
	 	                close(clientSocket);
	 	                return -1;
	                }
		    	
		    	info.fileName = sourceFileName;

		    	pthread_mutex_lock(&mapMutex);
		    	gradingRequestMap[requestID] = info;
		    	pthread_mutex_unlock(&mapMutex);
		    	
		    	string result_msg = "Your grading request ID " + requestID + " has been accepted and is currently being processed";
		
		    	if(send(clientSocket, result_msg.c_str(), strlen(result_msg.c_str()), 0) == -1){
				perror("Error sending result message");
				close(clientSocket);
				return -1;
        		}
        		
        		pthread_cond_signal(&queueReady);
     		}
     		else{
     			bzero(buffer, BUFFER_SIZE);
			bytes_read = recv(clientSocket, buffer, BUFFER_SIZE,0);
	    		if (bytes_read <= 0) {
		    		cerr << "Error receiving requestID." << endl;
		    		close(clientSocket);
		    		return -1;
	     		}
	     		
	     		string requestID = string(buffer);
	     		
	     		string result_msg;
	     		if(gradingRequestMap.find(requestID) == gradingRequestMap.end()){
	     			result_msg = searchLog("gradingLog.txt", requestID);
	     		}
	     		else{
	     			result_msg = "Your grading request ID " + requestID + " has been accepted. It is currently at  position " + to_string(gradingRequestMap[requestID].queuePos)  
	     			+ " in the queue.";
	     		}
		    	if(send(clientSocket, result_msg.c_str(), strlen(result_msg.c_str()), 0) == -1){
				perror("Error sending result message");
				close(clientSocket);
				return -1;
        		}	
     		}
        		
        }
        
        pthread_mutex_unlock(&queueMutex);
    }
    close(serverSocket);

    return 0;
}


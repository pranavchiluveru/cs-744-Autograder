#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>
#include <chrono> 
#include <errno.h> 

using namespace std;

const int MAX_BUFFER_SIZE = 1024;

int main(int argc, char* argv[]) {
    if (argc != 6) {
        cerr << "Usage: " << argv[0] << " <serverIP:port> <sourceCodeFileTobeGraded> <loopNum> <sleepTimeSeconds> <timeout-seconds>" << endl;
        return 1;
    }

    char* serverAddress = strtok(argv[1], ":");
    char* serverPort = strtok(NULL, ":");
    char* CodeFile = argv[2];
    int loopNum = atoi(argv[3]);
    int sleepTimeSeconds = atoi(argv[4]);
    int timeoutSeconds = atoi(argv[5]); 
    
    
    double totalTime = 0.0; // To keep track of the total time taken
    int successfulResponses = 0; // To count the number of successful responses
  

    int numTimeouts=0,numErrors=0;
    
    struct timeval timeout;
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;

    uint64_t loopst_ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

    for (int i = 0; i < loopNum; ++i) {
        
        int clientSocket;
        struct sockaddr_in serverAddr;

        // Create socket
        if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            numErrors++;
            cerr << "Error creating socket." << endl;
            return 1;
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(atoi(serverPort));
        serverAddr.sin_addr.s_addr = inet_addr(serverAddress);

        // Connect to the server
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
		    cerr << "Connection timeout." << endl;
		    numTimeouts++;
	    }
	    else{
                    cerr << "Error connecting to server." << endl;
                    numErrors++;
            }
            close(clientSocket);
            return 1;
        }
        
        // Set the socket timeout
        if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
            cerr<<"ERROR setting socket timeout"<<endl;

        // Read the source code file
        ifstream file(CodeFile);
        string sourceCode((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close();

        // Send the source code to the server
        uint64_t st_ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        ssize_t bytesSent = send(clientSocket, sourceCode.c_str(), sourceCode.size(), 0);
        if (bytesSent == -1) {
            if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
		    cerr << "Connection timeout." << endl;
		    numTimeouts++;
	    } 
	    else{
                    cerr << "Error sending data." << endl;
                    numErrors++;
            }
            close(clientSocket);
            return 1;
        }

        char buffer[MAX_BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));

        // Receive result from the server
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
         
        // Record the end time
        uint64_t et_ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        if (bytesRead == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN){// Handle the timeout error
			cerr<<"Connection timeout."<<endl;
			numTimeouts++;
	    }
	    else{
                        cerr << "Error receiving data." << endl;
                        numErrors++;
            }
            close(clientSocket);
            return 1;
        }
        else
        {
          // Check for successful response 
        // If it's successful, increment the counter
            successfulResponses++;
        }
       
        // Calculate the response time in microseconds
        totalTime += (et_ms-st_ms);
        
        cout << buffer << endl;
        
        // Sleep for the specified time
        sleep(sleepTimeSeconds);
        
        // Close the client socket
        close(clientSocket);
    }
    uint64_t loopet_ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    float loopTime = loopet_ms-loopst_ms;

    // Output the results
    
    cout << "Total Loop Time: " << loopTime << " ms" << endl;
    cout << "Timeout Count: " << numTimeouts << endl;
    cout << "Error Count: " << numErrors << endl;
    cout << "Successful Response Count: " << successfulResponses << endl;
    cout << "Average Response Time: " << totalTime/loopNum << endl;

    return 0;
}


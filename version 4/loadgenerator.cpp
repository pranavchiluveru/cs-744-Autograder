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

const int BUFFER_SIZE = 1024; 
const int MAX_FILE_SIZE_BYTES = 4;
const int MAX_TRIES = 5;

//Utility Function to send a file of any size to the grading server
int send_file(int sockfd, char* file_path)
//Arguments: socket fd, file name (can include path)
{
    char buffer[BUFFER_SIZE]; //buffer to read  from  file
    bzero(buffer, BUFFER_SIZE); //initialize buffer to all NULLs
    FILE *file = fopen(file_path, "rb"); //open the file for reading, get file descriptor 
    if (!file)
    {
        cerr<<"Error opening file"<<endl;
        return -1;
    }
		
    //for finding file size in bytes
    fseek(file, 0L, SEEK_END); 
    int file_size = ftell(file);
    cout<<"File size is: "<< file_size<<endl;
    
    //Reset file descriptor to beginning of file
    fseek(file, 0L, SEEK_SET);
		
		//buffer to send file size to server
    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    //copy the bytes of the file size integer into the char buffer
    memcpy(file_size_bytes, &file_size, sizeof(file_size));
    
    //send file size to server, return -1 if error
    if (send(sockfd, &file_size_bytes, sizeof(file_size_bytes), 0) == -1)
    {
        cerr<<"Error sending file size"<<endl;
        fclose(file);
        return -1;
    }

	//now send the source code file 
    while (!feof(file))  //while not reached end of file
    {
    
    		//read buffer from file
        ssize_t bytes_read = fread(buffer, 1, BUFFER_SIZE -1, file);
        
     		//send to server
        if (send(sockfd, buffer, bytes_read+1, 0) == -1)
        {
            cerr<<"Error sending file data"<<endl;
            fclose(file);
            return -1;
        }
        
        //clean out buffer before reading into it again
        bzero(buffer, BUFFER_SIZE);
    }
    //close file
    fclose(file);
    return 0;
}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <new|status> <serverIP:port> <sourceCodeFileTobeGraded|requestID> " << endl;
        return 1;
    }

    char* requestType = argv[1];
    char* serverAddress = strtok(argv[2], ":");
    char* serverPort = strtok(NULL, ":");
    char* file_path = NULL, * requestID = NULL;
    
    if(strcmp(requestType, "new") == 0)	
    	file_path = argv[3];
    else			
    	requestID = argv[3];
    
    
    double totalTime = 0.0; // To keep track of the total time taken
    int successfulResponses = 0; // To count the number of successful responses
    int errCount=0;
    

    uint64_t loopst_ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

    
    int clientSocket;
    struct sockaddr_in serverAddr;

	// Create socket
	if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		errCount++;
		cerr << "Error creating socket." << endl;
		return 1;
	}

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(atoi(serverPort));
        serverAddr.sin_addr.s_addr = inet_addr(serverAddress);

        // Connect to the server
        int tries = 0;
        while (true)
        {
    	  //connect to the server using the socket fd created earlier
            if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == 0)
                break;
            sleep(1);
            tries += 1;
            if (tries == MAX_TRIES)
            {
                cout<<"Server not responding"<<endl;
                return -1;
            }
        }
        
         char buffer[BUFFER_SIZE]; //buffer into which we read  the received file chars
         uint64_t st_ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        
        //send requestType
        ssize_t bytesSent = send(clientSocket,requestType, strlen(requestType), 0);
	if (bytesSent <= 0) {
	    cerr << "Error sending requestType." << endl;
	    errCount++;
	    close(clientSocket);
	    return -1;
	}
	
        if(strcmp(requestType, "new") == 0){
		 //send the file by calling the send file utility function
		if (send_file(clientSocket, file_path) != 0){
		    cout<<"Error sending source file"<<endl;
		    close(clientSocket);
		    errCount++;
		    return -1;
		}
	}
	else{
    		bzero(buffer, BUFFER_SIZE);
		strcpy(buffer,requestID);
		bytesSent = send(clientSocket, buffer, BUFFER_SIZE, 0);
		if (bytesSent <= 0) {
		    cerr << "Error sending requestID." << endl;
		    errCount++;
		    close(clientSocket);
		}
	}

        // Receive result from the server
        ssize_t bytes_read;
        memset(buffer,0,BUFFER_SIZE);
	//read server response
	bytes_read = recv(clientSocket, buffer, BUFFER_SIZE, 0);

	if (bytes_read <= 0) {
	    cerr << "Error receiving status." << endl;
	    errCount++;
	    close(clientSocket);
	}
	
	cout<< buffer<<endl << flush;
	memset(buffer,0,BUFFER_SIZE);
        
         // Record the end time
        uint64_t et_ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        
        // Close the client socket
        close(clientSocket);
   

    return 0;
}


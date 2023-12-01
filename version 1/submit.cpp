#include<iostream>
#include<cstring>
#include<stdlib.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fstream>
#include<chrono> 

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
        size_t bytes_read = fread(buffer, 1, BUFFER_SIZE -1, file);
        
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
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " <serverIP:port> <sourceCodeFileTobeGraded> <loopNum> <sleepTimeSeconds>" << endl;
        return 1;
    }

    char* server_ip = strtok(argv[1], ":");
    int server_port = atoi(strtok(NULL, ":"));
    char* file_path = argv[2];
    int loopNum = atoi(argv[3]);
    int sleepTimeSeconds = atoi(argv[4]);
    
    double totalTime = 0.0; // To keep track of the total time taken
    int successfulResponses = 0; // To count the number of successful responses

    uint64_t loopst_ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

    for (int i = 0; i < loopNum; ++i) {
        
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            cerr<<"Socket creation failed"<<endl;
            return -1;
        }
        
        struct sockaddr_in serv_addr;
        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_ip, &serv_addr.sin_addr.s_addr);

        // Connect to the server
        int tries = 0;
        while (true)
        {
    	  //connect to the server using the socket fd created earlier
            if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0)
                break;
            sleep(1);
            tries += 1;
            if (tries == MAX_TRIES)
            {
                cout<<"Server not responding"<<endl;
                return -1;
            }
        }
        
        
        // Send the source code to the server
        uint64_t st_ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        
        //send the file by calling the send file utility function
        if (send_file(sockfd, file_path) != 0)
        {
            cout<<"Error sending source file"<<endl;
            close(sockfd);
            return -1;
        };
    
        size_t bytes_read;
        //buffer for reading server response
        char buffer[BUFFER_SIZE];
        memset(buffer,0,BUFFER_SIZE);
        while (true)
        {

    	    //read server response
            bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0);
            
            if (bytes_read <= 0)
            {
                break;
            }
            cout<< buffer<<endl;
            memset(buffer,0,BUFFER_SIZE);
        }
        uint64_t et_ms  = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        
        successfulResponses++;
        // Calculate the response time in microseconds
        totalTime += (et_ms-st_ms);
        
        cout << buffer << endl;

        // Sleep for the specified time
        sleep(sleepTimeSeconds);
        
        //close socket file descriptor
        close(sockfd);
        
    }
    
    uint64_t loopet_ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    
    double TotalLoopTime=loopet_ms-loopst_ms; 
    float throughput=(successfulResponses/TotalLoopTime)*1000;

    cout << "Total Loop Time: " << TotalLoopTime << " milliseconds" << endl;
    // Calculate the average response time
    double averageResponseTime = (totalTime /loopNum );

    // Output the results
    cout << "Average Response Time: " << averageResponseTime << " milliseconds" << endl;
    cout << "Number of Successful Responses: " << successfulResponses << endl;
    cout << "Total Time Taken: " << totalTime << " milliseconds" << endl;
    cout << "Throughput: " << throughput<< endl;
    return 0;
    
}

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std; // Add this line to avoid using std:: everywhere

const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;

void error(char *msg) {
  perror(msg);
  exit(1);
}

string generateUniqueID() {
    time_t t;
    struct tm* now;
    char timestamp[20];
    time(&t);
    now = localtime(&t);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", now);
    // Generate a random number between 0 and 9999
    int randomNum = rand() % 10000;
    // Append the random number to the timestamp
    string uniqueID = timestamp + to_string(randomNum);
    return uniqueID;
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

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <port>" << endl;
        return 1;
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
    if (listen(serverSocket, 5) == -1) {
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
        
    	string id=generateUniqueID();
    	// Generate unique filenames for received code and temporary files
        string uniqueFileName = "code_" + id + ".cpp";
        string uniqueExecutableName = "executable_" + id;
        string compileErrorFileName = "compile_error_" + id + ".txt";
        string outputFileName = "output_" + id + ".txt";
        string outputDiffFileName = "output_diff_" + id + ".txt";
        string runErrorFileName= "run_error_"+ id + ".txt";
        
        // Receive source code from client
        if (recv_file(clientSocket, uniqueFileName.c_str()) != 0){
         	close(clientSocket);
         	continue;
    	}
    	
    	string compile_command = "g++ -o "+uniqueExecutableName +" "+uniqueFileName +" > /dev/null 2> "+compileErrorFileName; 
    	string run_command = "./"+uniqueExecutableName+" > "+outputFileName+" 2> "+runErrorFileName;
    	string compare_command = "diff "+outputFileName+" expected_output.txt > "+outputDiffFileName;
    	
    	string result_msg = "";
    	string result_filename = "";

        // Compile 
        if (system(compile_command.c_str()) != 0) {
            // Compilation failed
            result_msg = "COMPILER ERROR\n";
            result_filename = compileErrorFileName;
        }
        else if (system(run_command.c_str()) != 0) {
            // runtime error
            result_msg = "RUNTIME ERROR\n";
            result_filename = runErrorFileName ;
        }
	else if (system(compare_command.c_str()) != 0) {
            // output error
            result_msg = "OUTPUT ERROR\n";
            result_filename = outputDiffFileName ;
        }
        else{
        	result_msg = "PASS\n"; 	
        }

        if(send(clientSocket, result_msg.c_str(), strlen(result_msg.c_str()), 0) == -1){
        	perror("Error sending result message");
        	close(clientSocket);
        }

        if(!result_filename.empty()){
        	char buffer[BUFFER_SIZE];
        	memset(buffer, 0, sizeof(buffer));
        	
        	FILE *file = fopen(result_filename.c_str(),"rb");
        	while(!feof(file)){
        		size_t bytes_read = fread(buffer,1,sizeof(buffer),file);
        		if(send(clientSocket,buffer,bytes_read,0) ==-1){
        			perror("Error sending result message");
        			close(clientSocket);
        		} 
        		
        		memset(buffer, 0, sizeof(buffer));
        	}
        	
        	fclose(file);
        }
        remove(uniqueFileName.c_str());
        remove(uniqueExecutableName.c_str());
        remove(compileErrorFileName.c_str());
        remove(outputFileName.c_str());
        remove(outputDiffFileName.c_str());
        remove(runErrorFileName.c_str());
        
        close(clientSocket);
    }

    return 0;
}


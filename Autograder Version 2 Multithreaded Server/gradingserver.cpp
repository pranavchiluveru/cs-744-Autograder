#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ctime>
#include <sstream>
#include <pthread.h>
#include <vector>

using namespace std;

const int BUFFER_SIZE = 1024;

// Create a structure to hold client socket and any other necessary data
struct ThreadArgs {
    int clientSocket;
};

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

void *handleClient(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int clientSocket = args->clientSocket;

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    
    // Receive source code from the client
    ssize_t sourceBytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (sourceBytesRead == -1) {
        cerr << "Error receiving data." << endl;
        close(clientSocket);
        delete args;
        pthread_exit(NULL);
    }
      
    // Generate unique filenames for received code and temporary files
    string uniqueFileName = "code_" + generateUniqueFileName() + ".cpp";
    string uniqueExecutableName = "code_" + generateUniqueFileName();
    string compileErrorFileName = "compile_error_" + uniqueExecutableName + ".txt";
    string outputFileName = "output_" + uniqueExecutableName + ".txt";
    string outputDiffFileName = "output_diff_" + uniqueExecutableName + ".txt";

    // Save received code to the unique filename
    FILE* codeFile = fopen(uniqueFileName.c_str(), "w");
    fwrite(buffer, 1, sourceBytesRead, codeFile);
    fclose(codeFile);
    

    // Compile and run the code
    int compileResult = system(("g++ -o " + uniqueExecutableName + " " + uniqueFileName + " 2> " + compileErrorFileName).c_str());
    
    // Clean up temporary files
    remove(uniqueFileName.c_str());
    
    if (compileResult != 0) {
        // Compilation failed, send the error message back
        FILE* errorFile = fopen(compileErrorFileName.c_str(), "r");
        char compileErrorBuffer[BUFFER_SIZE];
        memset(compileErrorBuffer, 0, sizeof(compileErrorBuffer));
        size_t compileErrorBytesRead = fread(compileErrorBuffer, 1, sizeof(compileErrorBuffer) - 1, errorFile);
        string response = "COMPILER ERROR\n";
        response += compileErrorBuffer;
        send(clientSocket, response.c_str(), response.size(), 0);
        fclose(errorFile);

        // Clean up remaining temporary files
        remove(compileErrorFileName.c_str());
        close(clientSocket);
        delete args;
        pthread_exit(NULL);
    }
    else{
        int executionResult = system(("./" + uniqueExecutableName + " > " + outputFileName + " 2>&1").c_str());
        // Clean up the executable file
        remove(uniqueExecutableName.c_str());

        if (executionResult != 0) {
            FILE* errorFile = fopen(outputFileName.c_str(), "r");
            if (errorFile != nullptr) {
                char runError[BUFFER_SIZE];
                memset(runError, 0, sizeof(runError));
                size_t runErrorBytesRead = fread(runError, 1, sizeof(runError), errorFile);
                fclose(errorFile);

                string response = "RUNTIME ERROR\n" + string(runError);
                send(clientSocket, response.c_str(), response.size(), 0);
            } else {
                string response = "RUNTIME ERROR: Failed to read error message.";
                send(clientSocket, response.c_str(), response.size(), 0);
            }
            // Clean up remaining temporary files
            remove(compileErrorFileName.c_str());
            remove(outputFileName.c_str());
            close(clientSocket);
            delete args;
            pthread_exit(NULL);
        } 
        else {
            // Capture the program's output
            FILE* outputFile = fopen(outputFileName.c_str(), "r");
            char outputBuffer[BUFFER_SIZE];
            memset(outputBuffer, 0, sizeof(outputBuffer));
            size_t outputBytesRead = fread(outputBuffer, 1, sizeof(outputBuffer) - 1, outputFile);
            fclose(outputFile);
            string programOutput(outputBuffer);

            // Check if the output is correct
            bool outputCorrect = isOutputCorrect(programOutput);

            // Send the result back to the client
            string response = outputCorrect ? "PASS" : "OUTPUT ERROR";
            if (response == "PASS") {
                send(clientSocket, response.c_str(), response.size(), 0);
                remove(compileErrorFileName.c_str());
            } 
            else {
                system(("diff expected_output.txt " + outputFileName + " > " + outputDiffFileName).c_str());
                FILE* outputDiffFile = fopen(outputDiffFileName.c_str(), "r");
                char outputDiffBuffer[BUFFER_SIZE];
                memset(outputDiffBuffer, 0, sizeof(outputDiffBuffer));
                size_t outputBytesRead = fread(outputDiffBuffer, 1, sizeof(outputDiffBuffer) - 1, outputDiffFile);
                fclose(outputDiffFile);
                response += string(outputDiffBuffer);
                send(clientSocket, response.c_str(), response.size(), 0);
            }

            // Clean up remaining temporary files
            remove(compileErrorFileName.c_str());
            remove(outputFileName.c_str());
            remove(outputDiffFileName.c_str());
        }
    }
    close(clientSocket);
    delete args;
    pthread_exit(NULL);
      
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
    if (listen(serverSocket, 10) == -1) {
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

        // Create a new ThreadArgs structure to handle this client
        ThreadArgs *args = new ThreadArgs;
        args->clientSocket = clientSocket;

        // Create a new thread to handle this client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handleClient, args) != 0) {
            cerr << "Failed to create thread." << endl;
        }
        
        // Detach the thread to automatically clean up when it exits
        pthread_detach(thread);
    }
    close(serverSocket);


    return 0;
}


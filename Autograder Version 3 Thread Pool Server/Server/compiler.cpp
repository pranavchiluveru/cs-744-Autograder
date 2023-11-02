#include "utilities.h"
#include "compiler.h"
#include <unistd.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <string>
using namespace std;

void compileAndRunCode(char* buffer, ssize_t sourceBytesRead, void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int clientSocket = args->Socket;


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
        //close(clientSocket);
        //delete args;
        //pthread_exit(NULL);
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
   //pthread_exit(NULL);
}


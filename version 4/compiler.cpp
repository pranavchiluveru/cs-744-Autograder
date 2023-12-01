#include "utilities.h"
#include "compiler.h"
#include <unistd.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <string>
using namespace std;



void compileAndRunCode(string sourceFileName, string requestID) {  
        
    	// Generate unique filenames for received code and temporary files
        string uniqueExecutableName = "executable_" + requestID;
        string compileErrorFileName = "compile_error_" + requestID+ ".txt";
        string outputFileName = "output_" +requestID + ".txt";
        string outputDiffFileName = "output_diff_" + requestID + ".txt";
        string runErrorFileName= "run_error_"+ requestID+ ".txt";
    
        string compile_command = "g++ -o "+uniqueExecutableName +" "+sourceFileName +" > /dev/null 2> "+compileErrorFileName; 
    	string run_command = "./"+uniqueExecutableName+" > "+outputFileName+" 2> "+runErrorFileName;
    	string compare_command = "diff "+outputFileName+" expected_output.txt > "+outputDiffFileName;
    	
    	string result_msg = "";
    	string result_filename = "";

        // Compile 
        if (system(compile_command.c_str()) != 0) {
            // Compilation failed
            result_msg = "Processing is done.\nCOMPILER ERROR\n";
            result_filename = compileErrorFileName;
        }
        else if (system(run_command.c_str()) != 0) {
            // runtime error
            result_msg = "Processing is done.\nRUNTIME ERROR\n";
            result_filename = runErrorFileName ;
        }
	else if (system(compare_command.c_str()) != 0) {
            // output error
            result_msg = "Processing is done.\nOUTPUT ERROR\n";
            result_filename = outputDiffFileName ;
        }
        else{
        	result_msg = "Processing is done.\nPASS\n"; 	
        }
        
        appendLogEntry(requestID, result_msg, result_filename);
        
        pthread_mutex_lock(&mapMutex);
	gradingRequestMap.erase(requestID);
	pthread_mutex_unlock(&mapMutex); 

       
        // Remove the temporary files
        remove(uniqueExecutableName.c_str());
        remove(compileErrorFileName.c_str());
        remove(outputFileName.c_str());
        remove(outputDiffFileName.c_str());
        remove(runErrorFileName.c_str());
}


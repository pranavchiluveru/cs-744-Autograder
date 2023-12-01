# cs-744-Autograder
In this Design and Engineering of Computer Systems course project we will step by step build a scalable program autograding server and its client

Version 4-Asynchronous Grading Architecture

use Makefile : command make
All: submit server
submit: loadgenerator.cpp
        g++ -o submit loadgenerator.cpp

server: gradingServer.cpp compiler.cpp utilities.cpp utilities.h compiler.h
        g++ -o server gradingServer.cpp compiler.cpp utilities.cpp
        
Run server:
./server <port> <thread_pool_size>

Run client:
./submit <new|status> <serverIP:port> <sourceCodeFileTobeGraded|requestID> 

OR for loadtesting : ./loadtest.sh



Version 3-Thread Pools 
Use makefile : command make
All: submit server
submit: loadgenerator.cpp
        g++ -o submit loadgenerator.cpp

server: gradingServer.cpp compiler.cpp utilities.cpp client_handler.cpp utilities.h compiler.h client_handler.h
        g++ -o server gradingServer.cpp compiler.cpp utilities.cpp client_handler.cpp
        
Run server:
./server <port> <thread_pool_size>

Run client :
./submit <serverIP:port> <sourceCodeFileTobeGraded> <loopNum> <sleepTimeSeconds> <timeout-seconds>

OR for loadtesting: ./analysis.sh




Version 2-MultiThreaded Server with Create-Destroy Threads
run gradingserver using: ./server <portno>
run client : ./submit <serverIP:port> <sourceCodeFileTobeGraded> <loopNum> <sleepTimeSeconds> <timeout-seconds>

To compile:
g++ -o submit loadgeneratorMT.cpp
g++ -o server gradingServerMT.cpp




Version 1-Single Threaded Server
Run server: ./server <port>
run client : ./submit <serverIP:port> <sourceCodeFileTobeGraded> <loopNum> <sleepTimeSeconds>

To compile:
g++ -o submit submit.cpp
g++ -o server gradingServer.cpp




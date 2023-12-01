All: submit server
submit: loadgenerator.cpp
	g++ -o submit loadgenerator.cpp

server: gradingServer.cpp compiler.cpp utilities.cpp utilities.h compiler.h
	g++ -o server gradingServer.cpp compiler.cpp utilities.cpp

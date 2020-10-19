all: server client 

puzzle: server.cpp
	g++ -Wall -std=c++11 server.cpp -o server

scanner: client.cpp
	g++ -pthread client.cpp -o client
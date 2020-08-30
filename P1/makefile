server client: server.o client.o
	g++ server.o -o server
	g++ client.o -o client

server.o: server.cpp
	g++ -c -std=c++11 server.cpp

client.o: client.cpp
	g++ -c -std=c++11 client.cpp

clean:
	rm *.o 
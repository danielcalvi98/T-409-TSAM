/*
 * 
 * Simple chat server for TSAM-409
 * 
 * Command line: ./chat_server 4000 
 * 
 * Author: Jacky Mallett (jacky@ru.is)
 * 
 * Improved on and modified by:
 * 
 * Ymir Thorleifsson
 * 
 * Kristofer Robertsson
 * 
 */ 

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>

#include <iostream>
#include <sstream>
#include <thread>
#include <map>

#include <unistd.h>


#include <time.h>  // get time
#include <stack>   // stack
#include <fstream> // to write to file

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG  5          // Allowed length of queue of waiting connections

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Client {
  public:
    int sock;              // socket of client connection
    std::string name;           // Limit length of name of client's user

    Client(int socket) : sock(socket){} 

    ~Client(){}            // Virtual destructor defined for base class
};

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table, 
// (indexed on socket no.) sacrificing memory for speed.

std::map<int, Client*> servers; // Lookup table for per Client information

std::stack<int> remove_servers;

std::ofstream server_log;

// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.

int open_socket(int portno) {
    struct sockaddr_in sk_addr;   // address settings for bind()
    int sock;                     // socket opened for this port
    int set = 1;                  // for setsockopt

    // Create socket for connection. Set to be non-blocking, so recv will
    // return immediately if there isn't anything waiting to be read.
#ifdef __APPLE__     
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Failed to open socket");
        return(-1);
    }
#else
    if((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
        perror("Failed to open socket");
        return(-1);
    }
#endif

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after 
    // program exit.

    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0) {
        perror("Failed to set SO_REUSEADDR:");
    }
    set = 1;
#ifdef __APPLE__     
    if(setsockopt(sock, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0) {
        perror("Failed to set SOCK_NOBBLOCK");
    }
#endif
    memset(&sk_addr, 0, sizeof(sk_addr));

    sk_addr.sin_family      = AF_INET;
    sk_addr.sin_addr.s_addr = INADDR_ANY;
    sk_addr.sin_port        = htons(portno);

    // Bind to socket to listen for connections from clients

    if(bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0) {
        perror("Failed to bind to socket:");
        return(-1);
    } else {
        return(sock);
    }
}

// Get system time to string

std::string getSystemTime() {
    time_t      now = time(0);
    struct tm   tstruct;
    char        buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    return buf;
}

// custom print

void print(const std::string string) {
    std::string str = getSystemTime() + std::string(" ") + string + std::string("\n");
    const char *cstr = str.c_str();
    printf("%s",cstr);
    server_log.open("server_log.txt", std::ios_base::app);
    server_log << cstr;
    server_log.close();
}

// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.

void closeClient(int clientSocket, fd_set *openSockets, int *maxfds) {
    close(clientSocket);
    // Remove client from the clients list
    remove_servers.push(clientSocket);
    // servers.erase(clientSocket);

    // If this client's socket is maxfds then the next lowest
    // one has to be determined. Socket fd's can be reused by the Kernel,
    // so there aren't any nice ways to do this.

    if(*maxfds == clientSocket) {
        for(auto const& p : servers) {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
    }

    // And remove from the list of open sockets.

    FD_CLR(clientSocket, openSockets);
}

// Process command from client on the server

std::string clientName(int clientSocket) {
    if (!servers[clientSocket]->name.empty()) 
        return servers[clientSocket]->name;
    else
        return std::string("CLIENT:") + std::to_string(clientSocket);
}

void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, char *buffer) {
    std::vector<std::string> tokens;
    std::string token;
    std::string log = clientName(clientSocket);
    
    // Split command from client into tokens for parsing
    std::stringstream stream(buffer);

    while(stream >> token)
        tokens.push_back(token);

    if((tokens[0].compare("CONNECT") == 0) && (tokens.size() == 2)) {
        servers[clientSocket]->name = tokens[1];
        log += std::string(" CONNECTED AS ") + std::string(tokens[1]);
        print(log);

    } else if(tokens[0].compare("LEAVE") == 0) {
        // Close the socket, and leave the socket handling
        // code to deal with tidying up servers etc. when
        // select() detects the OS has torn down the connection.
        log += std::string(" LEFT");
        print(log);
        closeClient(clientSocket, openSockets, maxfds);

    } else if(tokens[0].compare("WHO") == 0) {
        log += " WITH COMMAND WHO";
        print(log);
        // std::cout << "Who is logged on" << std::endl;
        std::string msg;

        for(auto const& names : servers) {
            msg += clientName(names.first) + ",";
        }
        
        if (msg.empty()) {
            msg = std::string("None,");
        }
        msg = std::string("SERVERS CONNECTED: ") + msg;
        // Reducing the msg length by 1 loses the excess "," - which
        // granted is totally cheating.
        send(clientSocket, msg.c_str(), msg.length()-1, 0);

    } else if((tokens[0].compare("MSG") == 0) && (tokens[1].compare("ALL") == 0)) {
        log += std::string(" SENDING MESSAGE TO ALL");
        print(log);

        // This is slightly fragile, since it's relying on the order
        // of evaluation of the if statement.
        std::string msg;
        for(auto i = tokens.begin()+2;i != tokens.end();i++) {
            msg += *i + " ";
        }
        msg = "MESSAGE FROM " + clientName(clientSocket) + ": " + msg;
        for(auto const& pair : servers) {
            send(pair.second->sock, msg.c_str(), msg.length(),0);
        }

    } else if(tokens[0].compare("MSG") == 0) {
        log += std::string(" SENDING MESSAGE TO ") + std::string(tokens[1]);
        print(log);
        for(auto const& pair : servers) {
            if(pair.second->name.compare(tokens[1]) == 0) {
                std::string msg;
                for(auto i = tokens.begin()+2;i != tokens.end();i++) {
                    msg += *i + " ";
                }
                msg = "MESSAGE FROM " + clientName(clientSocket) + ": " + msg;
                send(pair.second->sock, msg.c_str(), msg.length(),0);
            }
        }

    } else {
        log += std::string(" WITH AN UNKNOWN COMMAND");
    }
}

int main(int argc, char* argv[]) {
    bool finished;
    int listenSock;                 // Socket for connections to server
    int clientSock;                 // Socket of connecting client
    fd_set openSockets;             // Current open sockets 
    fd_set readSockets;             // Socket list for select()        
    fd_set exceptSockets;           // Exception socket list
    int maxfds;                     // Passed to select() as max fd in set
    struct sockaddr_in client;
    socklen_t clientLen;
    char buffer[1025];              // buffer for reading from servers

    std::string log;
    int allowed_connections = 2;

    if(argc != 2) {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }

    // Setup socket for server to listen to

    listenSock = open_socket(atoi(argv[1]));  // The server
    log = std::string("SERVER LISTENING ON PORT: ") + std::string(argv[1]);
    print(log);


    if(listen(listenSock, BACKLOG) < 0) {
        log = std::string("LISTEN FAILED ON PORT: ") + std::string(argv[1]);
        print(log);
        exit(0);
    } else {
        // Add listen socket to socket set we are monitoring
        FD_ZERO(&openSockets);
        FD_SET(listenSock, &openSockets);
        maxfds = listenSock;
    }

    finished = false;

    while(!finished) {
        // std::cout << "while" << std::endl;

        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        // Look at sockets and see which ones have something to be read()
        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if(n < 0) {
            log = getSystemTime() + " SELECT FAILED - SHUTING DOWN\n";
            perror(log.c_str());
            finished = true;
        } else {
            // First, accept  any new connections to the server on the listening socket
            if(FD_ISSET(listenSock, &readSockets)) {
                clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);
                if (servers.size() < allowed_connections) {
                    log = std::string("SERVER ACCEPT'S CLIENT:") + std::to_string(clientSock);
                    // Add new client to the list of open sockets
                    FD_SET(clientSock, &openSockets);

                    // And update the maximum file descriptor
                    maxfds = std::max(maxfds, clientSock);

                    // create a new client to store information.
                    servers[clientSock] = new Client(clientSock);

                    // log = std::string("CLIENT:") + std::to_string(clientSock) + std::string(" CONNECTED ON SERVER");
                } else {
                    std::string msg = "Maximum of " + std::to_string(allowed_connections) + " connections reached";
                    send(clientSock, msg.c_str(), msg.length(), 0);
                    close(clientSock);
                    log = std::string("SERVER DOES NOT ACCEPT'S CLIENT:") + std::to_string(clientSock) + std::string(" TO MANY CONNECTIONS");
                }
                // Decrement the number of sockets waiting to be dealt with
                n--;
                print(log);
                
            }
            while(n-- > 0) {
                // Now check for commands from servers
                for(auto const& pair : servers) {
                    if (pair.first == 0) { // was deleted? maybe
                        continue;
                    }
                    //     Client *client = servers[i];
                    Client *client = pair.second; // first would be int or id, second would be the client class
                    
                    if(FD_ISSET(client->sock, &readSockets)) {
                        // recv() == 0 means client has closed connection
                        if(recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0) {
                            log = client->name + std::string(" CLOSED CONNECTION:") + std::to_string(client->sock);
                            print(log);

                            closeClient(client->sock, &openSockets, &maxfds);

                        } else {
                            // We don't check for -1 (nothing received) because select()
                            // only triggers if there is something on the socket for us.
                        
                            clientCommand(client->sock, &openSockets, &maxfds, buffer);
                        }
                    }
                }
                while (!remove_servers.empty()) {
                    servers.erase(remove_servers.top());
                    remove_servers.pop();
                }
            }
        }
    }
    print("SERVER IS SHUTTING DOWN");
}

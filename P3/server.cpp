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
#include <regex>   // regex
#include <ifaddrs.h>// get ip
#include <net/if.h> // get ip

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG  5          // Allowed length of queue of waiting connections

#define MAX_HOSTS 15

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Client {
  public:
    int sock;               // socket of client connection
    std::string name = "";  // Limit length of name of client's user
    std::string ip = "";         
    int port = 0;       
    int incomming;

    Client(int socket) : sock(socket){} 

    ~Client(){}            // Virtual destructor defined for base class
};

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table, 
// (indexed on socket no.) sacrificing memory for speed.

std::map<int, Client*> servers; // Lookup table for per server information
std::stack<int> remove_servers;

std::map<int, Client*> clients; // Lookup table for per Client information
std::stack<int> remove_clients;

std::ofstream server_log;
std::ofstream write_message;

std::string HOST_NAME = "P3_Group_112";
std::string HOST_IP;
int HOST_PORT;

// Open socket for specified port.

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
    std::string str = getSystemTime() + " " + string + "\n";
    const char *cstr = str.c_str();
    printf("%s",cstr);
    server_log.open("server_log.txt", std::ios_base::app);
    server_log << cstr;
    server_log.close();
}

// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.

void closeServer(int clientSocket, fd_set *openSockets, int *maxfds) {
    close(clientSocket);
    remove_servers.push(clientSocket);

    if(*maxfds == clientSocket)
        for(auto const& p : servers)
            *maxfds = std::max(*maxfds, p.second->sock);
        
    FD_CLR(clientSocket, openSockets);
}

void closeClient(int clientSocket, fd_set *openSockets, int *maxfds) {
    close(clientSocket);
    // Remove client from the clients list
    remove_clients.push(clientSocket);
    // servers.erase(clientSocket);

    // If this client's socket is maxfds then the next lowest
    // one has to be determined. Socket fd's can be reused by the Kernel,
    // so there aren't any nice ways to do this.

    if(*maxfds == clientSocket) {
        for(auto const& p : clients) {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
    }

    // And remove from the list of open sockets.

    FD_CLR(clientSocket, openSockets);
}

// Process command from client on the server

void serverCommand(int clientSocket, fd_set *openSockets, int *maxfds, char *buffer, int buffersize) {
    std::string cli_name = servers[clientSocket]->name;
    std::string log = servers[clientSocket]->name;

    // Check if message is valid
    if (!std::regex_search(buffer, std::regex("\\*.*#"))) {
        log += " WITH AN UNKNOWN COMMAND";
        print(log);
        return;
    }

    // parse string
    std::vector<std::string> tokens;
    std::string token;
    bool start = false;
    for (int i = 0; i < buffersize; i++) {
        if (start) {
            if (buffer[i] == ',' || buffer[i] == ';' || buffer[i] == '#') {
                tokens.push_back(token);
                token = "";
                if (buffer[i] == '#') break;
                continue;
            }
            token += buffer[i];
        }
        if (buffer[i] == '*') start = true;
    }
    
    // COMMANDS
    if((tokens[0].compare("QUERYSERVERS") == 0) && (tokens.size() == 2)) {
        log = tokens[1] + " REQUESTING QUERYSERVERS";
        print(log);
        std::string msg = "*CONNECTED,";
        
        msg += HOST_NAME + ",";
        msg += HOST_IP + ",";
        msg += std::to_string(HOST_PORT) + ";";

        for(auto const& server : servers) {
            if (server.second->name.empty()) continue;
            msg += server.second->name + ",";
            msg += server.second->ip + ",";
            msg += std::to_string(server.second->port) + ";";
        }
        msg += "#";

        send(clientSocket, msg.c_str(), msg.length(), 0);

    } else if(tokens[0].compare("CONNECTED") == 0) {
        servers[clientSocket]->name = tokens[1];
        servers[clientSocket]->ip   = tokens[2];
        servers[clientSocket]->port = atoi(tokens[3].c_str());
        print(tokens[1] + " CONNECTED AS " + tokens[1]);

    } else if ((tokens[0].compare("KEEPALIVE") == 0) && (tokens.size() == 2)) {
        std::string log = cli_name + " REQUESTING KEEPALIVE";

        // Check if digit
        bool digit = true;
        for (char& chr : tokens[1]) {
            if (isdigit((int) chr)) continue;
            digit = false;
        }

        if (digit) {
            servers[clientSocket]->incomming = atoi(tokens[1].c_str());
        } else {
            log += " - WITH INVALID ARGUMENT";
        }
        print(log);

    } else if(tokens[0].compare("LEAVE") == 0) {
        // Close the socket, and leave the socket handling
        // code to deal with tidying up servers etc. when
        // select() detects the OS has torn down the connection.
        log += " LEFT";
        print(log);
        closeClient(clientSocket, openSockets, maxfds);

    } else if((tokens[0].compare("GET_MSG") == 0) && (tokens.size() == 2)) {
        print(cli_name + " REQUESTING MESSAGES FOR " + tokens[1]);
        
        for(auto const& pair : servers) {
            if (pair.second->name.compare(tokens[1]) == 0) {
                for (std::string message : messages[pair.second->sock])
                    send(pair.second->sock, msg.c_str(), msg.length(), 0);
                messages[clientSocket].clear();
            }
        }


    } else if((tokens[0].compare("SEND_MSG") == 0) && (tokens.size() > 2)) {
        print(tokens[2] + " SENDING MESSAGE TO " + tokens[1]);

        // Create msg
        std::string msg;
        for(auto i = tokens.begin()+2; i != tokens.end(); i++) msg += *i + " ";
        msg.pop_back(); // remove space

        // Save message
        for(auto const& pair : servers) {
            if (pair.second->name.compare(tokens[1]) == 0) {
                msg = "*SEND_MSG," + tokens[1] + "," + tokens[2] + "," + msg + "#";
                messages[pair.second->sock].add(msg);
            }
        }

    } else {
        print(cli_name + " WITH AN UNKNOWN COMMAND");
    }
}

void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, char *buffer, int buffersize, bool *finished) {
    std::vector<std::string> tokens;
    std::string token;
    std::string log = "CLIENT:" + std::to_string(clientSocket);

    // Split command from client into tokens for parsing
    std::stringstream stream(buffer);

    while(std::getline(stream, token, ','))
        tokens.push_back(token);

    tokens.back().pop_back(); // Remove null terminator on last element

    if(tokens[0].compare("SHUTDOWN") == 0) {
        log += " REQUESTING SHUTDOWN";
        print(log);
        finished[0] = true;

    } else if(tokens[0].compare("LEAVE") == 0) {
        log += " LEFT";
        print(log);
        closeClient(clientSocket, openSockets, maxfds);

    } else if(tokens[0].compare("LISTSERVERS") == 0) {
        log += " REQUESTING CONNECED SERVERS";
        print(log);
        std::string msg;

        for(auto const& names : servers)
            msg += names.second->name + ",";

        send(clientSocket, msg.c_str(), msg.length()-1, 0);

    } else if(tokens[0].compare("GETMSG") == 0  && (tokens.size() == 2)) {
        bool from_id = false;
        if (tokens[1].compare("ALL") != 0) 
            from_id = true;
        
        log += " REQUESTING MESSAGES";
        if (from_id) {
            log += " FROM GROUP " + tokens[1];
        }
        print(log);

        std::ifstream read_messages("messages_to_client.txt");
        for (std::string msg; std::getline(read_messages, msg); ) {
            msg +="\n"; // So client knows it is a line
            if (from_id && !std::regex_search(msg, std::regex("S3_Group_" + tokens[1])))
                continue;
            send(clientSocket, msg.c_str(), msg.length(), 0);
        }

    } else if(tokens[0].compare("SENDMSG") == 0) {
        log += " SENDING MESSAGE TO " + std::string(tokens[1]);
        print(log);
        for(auto const& pair : servers) {
            if(pair.second->name.compare(tokens[1]) == 0) {
                std::string msg;
                for(auto i = tokens.begin()+2; i != tokens.end(); i++) {
                    msg += *i + " ";
                }
                msg.pop_back();
                msg = "*SEND_MSG," + pair.second->name + "," + HOST_NAME + "," + msg + "#";
                send(pair.second->sock, msg.c_str(), msg.length(),0);
            }
        }

    } else {
        log += " WITH AN UNKNOWN COMMAND";
        print(log);
    }
}

void checkServerCommands(char *buffer, int buffersize, fd_set *openSockets, fd_set *readSockets, int *maxfds) {
    for(auto const& pair : servers) {
        if (pair.first == 0) { // was deleted
            continue;
        }
        //     Client *client = servers[i];
        Client *client = pair.second;
        
        if(FD_ISSET(client->sock, readSockets)) {
            // recv() == 0 means client has closed connection
            if(recv(client->sock, buffer, buffersize, MSG_DONTWAIT) == 0) {
                std::string log = client->name + " CLOSED CONNECTION:" + std::to_string(client->sock);
                print(log);

                closeServer(client->sock, openSockets, maxfds);

            } else {
                // We don't check for -1 (nothing received) because select()
                // only triggers if there is something on the socket for us.
                serverCommand(client->sock, openSockets, maxfds, buffer, buffersize);
            }
        }
    }
    while (!remove_servers.empty()) {
        servers.erase(remove_servers.top());
        remove_servers.pop();
    }
}

void checkClientCommands(char *buffer, int buffersize, fd_set *openSockets, fd_set *readSockets, int *maxfds, bool *finished) {
    
    for(auto const& pair : clients) {
        if (pair.first == 0) { // was deleted
            continue;
        }
        Client *client = pair.second; 
        
        if(FD_ISSET(client->sock, readSockets)) {
            // recv() == 0 means client has closed connection
            if(recv(client->sock, buffer, buffersize, MSG_DONTWAIT) == 0) {
                std::string log = client->name + " CLOSED CONNECTION:" + std::to_string(client->sock);
                print(log);

                closeClient(client->sock, openSockets, maxfds);

            } else {
                // We don't check for -1 (nothing received) because select()
                // only triggers if there is something on the socket for us.
            
                clientCommand(client->sock, openSockets, maxfds, buffer, buffersize, finished);
            }
        }
    }
    while (!remove_clients.empty()) {
        clients.erase(remove_clients.top());
        remove_clients.pop();
    }
}

std::string get_ip(){
    FILE *fpipe;
    const char *command = "hostname -I";
    char c = 0;

    fpipe = (FILE*) popen(command, "r");
    std::string ip;
    while (fread(&c, sizeof(c), 1, fpipe)) {
        if (c == ' ') break;
        ip += c;
    }
    pclose(fpipe);
    return ip;
}

int main(int argc, char* argv[]) {
    bool finished[2];
    int listenSock;                 // Socket for connections to server
    int clientSock;                 // Socket of connecting client
    fd_set openSockets;             // Current open sockets 
    fd_set readSockets;             // Socket list for select()        
    fd_set exceptSockets;           // Exception socket list
    int maxfds;                     // Passed to select() as max fd in set
    struct sockaddr_in client;
    socklen_t clientLen;
    char buffer[5000];              // buffer for reading from servers

    std::string log;
    int allowed_connections = 15;

    
    if(argc != 2) {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }

    HOST_IP = get_ip();
    HOST_PORT = atoi(argv[1]);

    // Setup socket for server to listen to

    listenSock = open_socket(atoi(argv[1]));  // The server
    log = "SERVER LISTENING ON PORT: " + std::string(argv[1]);
    print(log);

    if(listen(listenSock, BACKLOG) < 0) {
        log = "LISTEN FAILED ON PORT: " + std::string(argv[1]);
        print(log);
        exit(0);
    } else {
        // Add listen socket to socket set we are monitoring
        FD_ZERO(&openSockets);
        FD_SET(listenSock, &openSockets);
        maxfds = listenSock;
    }

    finished[0] = false;
    while(!finished[0]) {
        // std::cout << "while" << std::endl;

        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        // Look at sockets and see which ones have something to be read()
        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if(n < 0) {
            log = getSystemTime() + " SELECT FAILED - SHUTTING DOWN\n";
            perror(log.c_str());
            finished[0] = true;
        } else {
            // First, accept  any new connections to the server on the listening socket
            if(FD_ISSET(listenSock, &readSockets)) {
                clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);
                
                recv(clientSock, buffer, sizeof(buffer), MSG_DONTWAIT);
                std::string password = buffer;

                if (servers.size() < allowed_connections && clientSock > 0) {
                    log = "SERVER ACCEPT'S ";;
                    // Add new client to the list of open sockets
                    FD_SET(clientSock, &openSockets);

                    // And update the maximum file descriptor
                    maxfds = std::max(maxfds, clientSock);

                    // create a new client to store information.
                    if (password == "password1234") {
                        log += "CLIENT:" + std::to_string(clientSock);
                        clients[clientSock] = new Client(clientSock);
                    } else {
                        log += "SERVER:" + std::to_string(clientSock);
                        servers[clientSock] = new Client(clientSock);
                    }

                } else {
                    std::string msg = "Maximum of " + std::to_string(allowed_connections) + " connections reached";
                    send(clientSock, msg.c_str(), msg.length(), 0);
                    close(clientSock);
                    log = "SERVER DOES NOT ACCEPT'S CLIENT:";
                    if (clientSock > 0) {
                        log += std::to_string(clientSock) + " TO MANY CONNECTIONS";
                    } else {
                        log += " UNABLE TO SELECT CLIENT";
                    }
                }
                // Decrement the number of sockets waiting to be dealt with
                n--;
                print(log);
                
            }
            while(n-- > 0) {

                // Now check for commands from servers and clients
                checkServerCommands(buffer, sizeof(buffer), &openSockets, &readSockets, &maxfds);

                checkClientCommands(buffer, sizeof(buffer), &openSockets, &readSockets, &maxfds, finished);
            }
        }
    }
    print("SERVER IS SHUTTING DOWN");
}

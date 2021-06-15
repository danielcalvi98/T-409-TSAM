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
#include <chrono>  // get timeunits
#include <stack>   // stack
#include <fstream> // to write to file

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG  5          // Allowed length of queue of waiting connections

#define MAX_HOSTS 15

/** 
 * Simple class for handling connections from clients.
 * Client(int socket) - socket to send/receive traffic from client.
 */
class Client {
  public:
    int sock;               // socket of client connection
    std::string name = "";  // Limit length of name of client's user
    std::string ip = "";         
    int port = 0;
    std::string connected = "";

    Client(int socket) : sock(socket){} 

    ~Client(){}            // Virtual destructor defined for base class
};

std::map<int, Client*> servers; // Lookup table for per server information AKA 1 hop servers
std::map<int, Client*> clients; // Lookup table for per Client information
std::stack<int> removeStack; // Stack to remove servers, because it is bad practice to do it in a for loop
// std::stack<int> rem.ove_clients; // Stack to remove clients, because it is bad practice to do it in a for loop
std::map<std::string, std::vector<std::string>> messages; // Lookup table for per Client messages
std::vector<int> idleServers;           // Idle servers that will be kicked
std::ofstream server_log;               // File were the serverlog is dumped
std::string HOST_NAME = "P3_GROUP_112"; 
std::string HOST_IP;
int HOST_PORT;
int FINISHED = false; // Easier for all threads to know when to stop

/// Gets either Client from servers or from Clients @returns Client with socket
Client getClient(int socket) {
    if (servers.count(socket)) return *servers[socket];
    else                       return *clients[socket];
}

/**
 * Open socket for specified port.
 * @return -1 if unable to create the socket for any reason.
 */
int open_socket(int portno) {

    struct sockaddr_in sk_addr;   // address settings for bind()
    int sock;                     // socket opened for this port
    int set = 1;                  // for setsockopt

    // Create socket for connection. Set to be non-blocking, so recv will
    // return immediately if there isn't anything waiting to be read.
#ifdef __APPLE__     
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Failed to open socket");
        return(-1);
    }
#else
    if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
        perror("Failed to open socket");
        return(-1);
    }
#endif

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after 
    // program exit.

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0) {
        perror("Failed to set SO_REUSEADDR:");
    }
    set = 1;
#ifdef __APPLE__     
    if (setsockopt(sock, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0) {
        perror("Failed to set SOCK_NOBBLOCK");
    }
#endif
    memset(&sk_addr, 0, sizeof(sk_addr));

    sk_addr.sin_family      = AF_INET;
    sk_addr.sin_addr.s_addr = INADDR_ANY;
    sk_addr.sin_port        = htons(portno);

    // Bind to socket to listen for connections from clients

    if (bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0) {
        perror("Failed to bind to socket:");
        return(-1);
    } else {
        return(sock);
    }
}

/**
 * Gets the host's ip, using "hostname -I"
 * @return std::string ip
 */
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

/// Gets system time to string @returns Timestamp string.
std::string getSystemTime() {
    time_t      now = time(0);
    struct tm   tstruct;
    char        buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    return buf;
}

/// Custom print that adds timestamp and saves to file: "server_log.txt" @param string String that will be printed and saved.
void print(const std::string string) {
    std::string str = getSystemTime() + " " + string + "\n";
    const char *cstr = str.c_str();
    printf("%s",cstr);
    server_log.open("server_log.txt", std::ios_base::app);
    server_log << cstr;
    server_log.close();
}

/**
 * Receives a character array and parses it, with option to parse from server in that case 
 * it is also validated, because it has to start command with '*' and end with '#'.
 * 
 * @param buffer Character array sent from server
 * @param buffersize Size of the character array sent from server
 * @param valid Pointer to boolian variable
 * @param fromServer Option to parse from a server, lookfor '*' and '#'
 * 
 * @returns vector of tokens
 */
std::vector<std::string> parseString(char *buffer, int buffersize, bool *valid, bool fromServer) {
    std::vector<std::string> tokens;
    std::string token;

    bool start = false, end = false;
    for (int i = 0; i < buffersize; i++) {
        if (buffer[i] == '*' && fromServer) {start = true; continue;}
        if (buffer[i] == '\n' ||buffer[i] == ',' || buffer[i] == ';' || buffer[i] == '#' || buffer[i] == '\0') {
            if (!token.empty()) tokens.push_back(token);
            token = "";
            if (buffer[i] == '\0') break;
            if (buffer[i] == '#' && fromServer) {end = true; break;}
            continue;
        }
        token += buffer[i];
    }
    *valid = (start && end && !tokens.empty() && fromServer) || (!fromServer && !tokens.empty());

    return tokens;
}

/** 
 * Close a sockets's connection, add client/server to remove stack, and tidy up select sockets afterwards.
 * 
 * @param socket Socket that is being closed.
 * @param openSockets fd_set of all connected sockets.
 * @param maxfds The highest file descriptor we have open.
 */
void closeConnection(int socket, fd_set *openSockets, int *maxfds) {
    if (servers.count(socket)) servers.erase(socket);
    else                       clients.erase(socket);
    close(socket);
    if (*maxfds == socket) {
        for (auto const &pair : servers)
            *maxfds = std::max(*maxfds, pair.second->sock);
        for (auto const &pair : clients)
            *maxfds = std::max(*maxfds, pair.second->sock);
    }
    FD_CLR(socket, openSockets);
}

/**
 * Sends message "*CONNECTED,<HOST_NAME>,<HOST_IP>,<HOST_PORT>;<CONNECTED1_NAME>,
 * <CONNECTED1_IP>,<CONNECTED1_PORT>;...#" to socket.
 * 
 * @param socket The socket that is being sent to.
 */
void sendConnected(int socket) {
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
    send(socket, msg.c_str(), msg.length(), 0);
}

/**
 * Sends message "*QUERYSERVERS,<HOST_NAME>#" to socket.
 * 
 * @param socket The socket that is being sent to.
 */
void sendQueryServers(int socket) {
    print("SENDING QUERYSERVERS TO CLIENT:" + std::to_string(socket));
    std::string msg = "*QUERYSERVERS," + HOST_NAME + "#";
    send(socket, msg.c_str(), msg.length(), 0);
}

/**
 * Sends message "*KEEPALIVE,<keepalive>#" to socket.
 * 
 * @param socket The socket that is being sent to.
 * @param keepalive Number of messages that are waiting for client.
 */
void sendKeepAlive(int socket, int keepalive) {

    print("SENDING KEEPALIVE TO " + getClient(socket).name);
    std::string msg = "*KEEPALIVE," + std::to_string(keepalive) + "#";
    send(socket, msg.c_str(), msg.length(), 0);
}

/*------------- Server Commands -------------*/

/**
 * Recv message "*CONNECTED,<GROUP_NAME>,<GROUP_IP>,<GROUP_PORT>;<CONNECTED1_NAME>,
 * <CONNECTED1_IP>,<CONNECTED1_PORT>;...#" 
 * from other server. Sends connected, and sends query back if 
 * server is not identified.
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvConnected(int socket, std::vector<std::string> tokens, bool *valid) {
    if (tokens.size() < 4) {
        *valid = false;
        return;
    }
    servers[socket]->name = tokens[1];
    servers[socket]->ip   = tokens[2];
    servers[socket]->port = atoi(tokens[3].c_str());

    std::string connected = "*";
    for (auto it = begin (tokens); it != end (tokens); ++it) {
        connected += *it + ",";
    }
    connected.pop_back();
    connected += "#";
    servers[socket]->connected = connected;
    
    print("SERVER " + std::to_string(socket) + " CONNECTED AS " + tokens[1]);

}

/**
 * Recv message "*QUERYSERVERS,<FROM_GROUP_ID>#" from other server. 
 * Sends connected, and sends query back if server is not identified.
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvQueryServers(int socket, std::vector<std::string> tokens, bool *valid) {
    if (tokens.size() < 2) {
        *valid = false;
        return;
    }
    print(tokens[1] + " REQUESTING QUERYSERVERS");
    sendConnected(socket);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (servers[socket]->name.empty())
        sendQueryServers(socket);
}

/**
 * Recv message "*KEEPALIVE,<keepalive>#" from other server. Remove this server from 
 * idle servers and send *GETMSG,<HOST_NAME># if there are any messages.
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvKeepAlive(int socket, std::vector<std::string> tokens, bool *valid) {
    if (tokens.size() < 2) {
        *valid = false;
        return;
    }
    print(getClient(socket).name + " REQUESTING KEEPALIVE");
    
    for (auto it = idleServers.begin(); it < idleServers.end(); it++) {
        if (*it == socket) {
            idleServers.erase(it);
            break;
        }
    }

    if (tokens[1].compare("0") != 0) {
        std::string msg = "*GET_MSG," + HOST_NAME + "#";
        send(socket, msg.c_str(), msg.length(), 0);
    }
}

/**
 * Recv message "*SEND_MSG,<TO_GROUP_ID>,<FROM_GROUP_ID>,<MESSAGE_CONTENT>#" 
 * from other server. Saves the message.
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvSendMessage(int socket, std::vector<std::string> tokens, bool *valid) {
    if (tokens.size() <= 4) {
        *valid = false;
        return;
    }
    print(getClient(socket).name + " SENDING MESSAGE FROM " + tokens[2] + " TO " + tokens[1]);

    // Create msg
    std::string msg;
    for(auto i = tokens.begin()+3; i != tokens.end(); i++) msg += *i + " ";
    msg.pop_back(); // remove space
    msg = "*SEND_MSG," + tokens[1] + "," + tokens[2] + "," + msg + "#";

    // Save message
    messages[tokens[1]].push_back(msg);
}

/**
 * Recv message "*GET_MSG,<GROUP_ID>#" from other server. Sends all 
 * stored msg for server.
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvGetMessage(int socket, std::vector<std::string> tokens, bool *valid) {
    if (tokens.size() < 2) {
        *valid = false;
        return;
    }
    print(getClient(socket).name + " REQUESTING MESSAGES FOR " + tokens[1]);
    
    // Send all messages as that were requested
    if (messages.count(tokens[1])) {
        for (std::string msg: messages[tokens[1]]) 
            send(socket, msg.c_str(), msg.length(), 0);
        messages[tokens[1]].clear();
    }
}

/**
 * Recv "*LEAVE,<SERVER_IP>,<SERVER_PORT>#" from other server. Finds server 
 * connected on that IP and port and disconnects it.
 *
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvLeave(int socket, std::vector<std::string> tokens, bool *valid) {
    if (tokens.size() < 3) {
        *valid = false;
        return;
    }
    for(auto const& pair : servers) {
        if ((pair.second->ip.compare(tokens[1]) == 0) && (pair.second->port == atoi(tokens[2].c_str()))) {
            print(pair.second->name + " LEFT");
            removeStack.push(pair.second->sock);
        }
    }
}

/**
 * Recv "*STATUSREQ,<FROM_GROUP>#" from other server. Sends status response back.
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvStatusRequest(int socket, std::vector<std::string> tokens, bool *valid) {
    if (tokens.size() < 2) {
        *valid = false;
        return;
    }
    print(getClient(socket).name + " REQUESTING STATUSREQ");

    std::string msg = "*STATUSRESP," + HOST_NAME + "," + tokens[1];
    for(auto const& pair : messages) {
        if (pair.second.size() == 0) continue;
        msg += "," + pair.first;
        msg += "," + std::to_string(pair.second.size());
    }
    msg += "#";

    send(socket, msg.c_str(), msg.length(), 0);
}

/**
 * Recv "*STATUSRESP,<FROM_GROUP_1>,<TO_GROUP_2>,<NR_OF_MESSAGES_TO_GROUP_2>,...#" from other server. 
 * TODO: Check if we could forward messages for server. 
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvStatusResponse(int socket, std::vector<std::string> tokens, bool *valid) {
    if (tokens.size() < 3) {
        *valid = false;
        return;
    }
    print(getClient(socket).name + " SENDING STATUSRESP");
    for (auto it = begin (tokens) + 1; it < end (tokens); it += 2) {
        printf("%s",it->c_str());
    }
}

/*------------- Client Commands -------------*/

/**
 * Recv "SHUTDOWN" from client. Starts process to shutdown server.
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvShutdown(int socket, std::vector<std::string> tokens, bool *valid) {
    print(getClient(socket).name + " REQUESTING SHUTDOWN");
    std::string response = "SERVER IS SHUTTING DOWN";
    send(socket, response.c_str(), response.length(), 0);
    FINISHED = true;
}

/**
 * Recv "LEAVE" from client. Add to remove stack to remove and free up socket.
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvLeaveClient(int socket, std::vector<std::string> tokens, bool *valid) {
    print(getClient(socket).name + " LEFT");
    removeStack.push(socket);
}

/**
 * Recv "LISTSERVERS" from client. Checks all connected servers and sends them listed.
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvListServers(int socket, std::vector<std::string> tokens, bool *valid) {
    print(getClient(socket).name + " REQUESTING CONNECED SERVERS");
    std::string msg;

    for(auto const& names : servers)
        msg += names.second->name + ",";

    send(socket, msg.c_str(), msg.length()-1, 0);
}

/**
 * Recv "GETMSG,<FOR_GROUP_ID>" from client. 
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvGetMessageClient(int socket, std::vector<std::string> tokens, bool *valid) {
    if (tokens.size() < 2) {
        *valid = false;
        return;
    }
    print(getClient(socket).name + " REQUESTING A MESSAGE FOR " + tokens[1]);

    if (messages.count(tokens[1])) {
            std::string msg = messages[tokens[1]].front();
            send(socket, msg.c_str(), msg.length(), 0);
            messages[tokens[1]].erase(messages[tokens[1]].begin());
        if (messages[tokens[1]].empty()) {
            messages.erase(tokens[1]);
        }
    }
}

/**
 * Recv "SENDMSG,<TO_GROUP_ID>" from client. 
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvSendMessageClient(int socket, std::vector<std::string> tokens, bool *valid) {
    if (tokens.size() < 3) {
        *valid = false;
        return;
    }
    print(getClient(socket).name + " SENDING MESSAGE TO " + tokens[1]);
        
    std::string response = "MESSAGE SENT TO " + tokens[1];
    send(socket, response.c_str(), response.length(), 0);

    std::string msg;
    for(auto i = tokens.begin() + 2; i != tokens.end(); i++) 
        msg += *i + " ";
    msg.pop_back();
    msg = "*SEND_MSG," + tokens[1] + "," + HOST_NAME + "," + msg + "#";

    bool msgSent = false;
    for(auto const& pair : servers) {
        if (pair.second->name.compare(tokens[1]) == 0) {
            send(pair.second->sock, msg.c_str(), msg.length(), 0);
            msgSent = true;
            break;
        }
    }
    if (!msgSent) {
        messages[tokens[1]].push_back(msg);
    }
}

/**
 * Recv "FOR ME" from client. 
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvForMe(int socket, std::vector<std::string> tokens, bool *valid) {
    if (messages.count(HOST_NAME)) {
        std::map<std::string, int> forYou;
        for (auto &msg : messages[HOST_NAME]) {
            bool valid;
            auto tokens = parseString((char *) msg.c_str(), msg.length(), &valid, true);
            forYou[tokens[2]]++;
        }
        std::string response;
        for (auto const &pair : forYou) {
            response += pair.first + "," + std::to_string(pair.second) + ";";
        }
        if (response.empty())
            response = "NO MESSAGES";
        send(socket,response.c_str(), response.length(), 0);
    }
}

/**
 * Recv "STATUS" from client. 
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvStatus(int socket, std::vector<std::string> tokens, bool *valid) {
    std::string response;
    for (auto const &pair : messages) 
        response += pair.first + "," + std::to_string(pair.second.size()) + ";";
    if (response.empty())
        response = "NO MESSAGES";
    send(socket,response.c_str(), response.length(), 0);
}

/**
 * Recv "GET CONNECTED" from client. 
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvGetConnected(int socket, std::vector<std::string> tokens, bool *valid) {
    for (auto const& pair : servers) {
        if (pair.second->name.compare(tokens[1]) == 0) {
            send(socket,pair.second->connected.c_str(),pair.second->connected.length(), 0);
        }
    }
}

/**
 * Recv "CONNECT,<IP_ADDRESS>,<PORT>" from client. 
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 * @param valid Checks if tokens is of correct length and sets boolian flag
 */
void recvConnect(std::vector<std::string> tokens, fd_set *openSockets, int *maxfds) {
    std::string compareIP = tokens[1];
    if (compareIP.compare("LOCALHOST") == 0) compareIP = "127.0.0.1";
    if (compareIP.compare("127.0.0.1") == 0) compareIP = HOST_IP;
    
    // Don't allow duplicate connections
    for (auto const &pair : servers) {
        printf("%s %d\n",pair.second->ip.c_str(), pair.second->port);
        if ((pair.second->ip.compare(compareIP) == 0) && (std::to_string(pair.second->port).compare(tokens[2]) == 0)) {
            print("ALREADY CONNECTED TO SERVER ON IP: " + tokens[1] + ", PORT: " + tokens[2]);
            return;
        }
    }

    // make server connect
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    struct hostent *server;
    server = gethostbyname(tokens[1].c_str());

    bcopy((char *)server->h_addr,
        (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);

    serv_addr.sin_port = htons(atoi(tokens[2].c_str()));

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket <= 0) {
        print("FAILED TO CREATE SOCKET TO CONNECT TO SERVER ON IP: " + tokens[1] + ", PORT: " + tokens[2]);
        return;
    }

    int set = 1; 
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set));
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000; // 100 msec
    setsockopt(serverSocket, SOL_SOCKET, SO_SNDTIMEO, (char *) &tv, sizeof(tv));
    
    servers[serverSocket] = new Client(serverSocket); // Create new client before client has the chance to disconnect
    if ((connect(serverSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) >= 0) {
        print("CONNECTED TO SERVER ON IP: " + tokens[1] + ", PORT: " + tokens[2]);
        FD_SET(serverSocket, openSockets);

        // And update the maximum file descriptor
        *maxfds = std::max(*maxfds, serverSocket);

        sendQueryServers(serverSocket);
    } 
    else {
        servers.erase(serverSocket); // Erase client since it couldn't connect
        print("FAILED TO CONNECT TO SERVER ON IP: " + tokens[1] + ", PORT: " + tokens[2]);
    }
}

/**
 * Prints unknown command from server. 
 * 
 * @param socket The socket that the server, that is sending, is on.
 * @param tokens Tokenized message from server, easy to work with.
 */
void printUnknownCommand(int socket, std::vector<std::string> tokens) {
    std::string msg;
    for(auto i = tokens.begin(); i != tokens.end(); i++) msg += " " + *i;
    print(getClient(socket).name + " WITH AN UNKNOWN COMMAND:" + msg);
}

/*------------- Process Commands -------------*/

/**
 * Receives message from server, parses it and excecutes command if it is in list of commands, else discards it.
 * 
 * @param socket Socket of client that is sending request
 * @param buffer Character array sent from server
 * @param buffersize Size of the character array sent from server
 */
void serverCommand(int socket, char *buffer, int buffersize) {
    bool valid;
    std::vector<std::string> tokens = parseString(buffer, buffersize, &valid, true);
    if (!valid) {
        printUnknownCommand(socket, tokens);
        return;
    }
    
    std::map<std::string, void(*)(int, std::vector<std::string>, bool(*))> commands;
    commands["QUERYSERVERS"] = recvQueryServers;
    commands["CONNECTED"   ] = recvConnected;
    commands["KEEPALIVE"   ] = recvKeepAlive;
    commands["GET_MSG"     ] = recvGetMessage;
    commands["SEND_MSG"    ] = recvSendMessage;
    commands["LEAVE"       ] = recvLeave;
    commands["STATUSREQ"   ] = recvStatusRequest;
    commands["STATUSRESP"  ] = recvStatusResponse;

    valid = true;
    if (commands.count(tokens[0])){
        commands[tokens[0]](socket, tokens, &valid);
        if (valid) return;
    }

    printUnknownCommand(socket, tokens);
}

/**
 * Receives message from client, parses it and excecutes command if it is in list of commands, else discards it.
 * 
 * @param socket Socket of client that is sending request
 * @param openSockets fd_set of all connected sockets
 * @param maxfds The highest file descriptor we have open
 * @param buffer Character array sent from server
 * @param buffersize Size of the character array sent from server
 */
void clientCommand(int socket, fd_set *openSockets, int *maxfds, char *buffer, int buffersize) {
    bool valid;
    std::vector<std::string> tokens = parseString(buffer, buffersize, &valid, false);
    if (!valid) {
        std::string command_string;
        for(auto i = tokens.begin(); i != tokens.end(); i++) command_string += " " + *i;
        std::string help = HOST_NAME + ": " + command_string + ": COMMAND NOT FOUND, TRY 'HELP' FOR LIST OF COMMANDS";
        send(socket, help.c_str(), help.length(), 0);
        printUnknownCommand(socket, tokens);
        return;
    }

    std::map<std::string, void(*)(int, std::vector<std::string>, bool(*))> commands;
    commands["SHUTDOWN"     ] = recvShutdown;
    commands["LEAVE"        ] = recvLeaveClient;
    commands["LISTSERVERS"  ] = recvListServers;
    commands["GETMSG"       ] = recvGetMessageClient;
    commands["SENDMSG"      ] = recvSendMessageClient;
    commands["FOR ME"       ] = recvForMe;
    commands["STATUS"       ] = recvStatus;
    commands["GET CONNECTED"] = recvGetConnected;

    valid = true;
    if (commands.count(tokens[0])){
        commands[tokens[0]](socket, tokens, &valid);
        if (valid) return;
    }
    if ((tokens[0].compare("CONNECT") == 0) && (tokens.size() >= 3)) {
        recvConnect(tokens, openSockets, maxfds);
    } else if (tokens[0].compare("HELP") == 0) {
        std::string response = "AVAILABLE COMMANDS INCLUDE:\n";
        for (auto const &pair : commands) {
            response += "    " + pair.first + "\n";
        }
        response += "    CONNECT\n";
        response += "    HELP";
        send(socket, response.c_str(), response.length(), 0);
    } else {
        std::string command_string;
        for(auto i = tokens.begin(); i != tokens.end(); i++) command_string += " " + *i;
        std::string help = HOST_NAME + ": " + command_string + ": COMMAND NOT FOUND, TRY 'HELP' FOR LIST OF COMMANDS";
        send(socket, help.c_str(), help.length(), 0);
        printUnknownCommand(socket, tokens);
    }
}

/**
 * Loops through all servers and checks if they are set (have updated i.e. sent message, disconnected), 
 * and removes them or excecutes their command accordingly
 * 
 * @param buffer Character array sent from server
 * @param buffersize Size of the character array sent from server
 * @param openSockets fd_set of all connected sockets
 * @param readSockets fd_set of connected sockets, not including newly connected
 * @param maxfds The highest file descriptor we have open
 */
void checkServerCommands(char *buffer, int buffersize, fd_set *readSockets) {
    for(auto const& pair : servers) {
        if (pair.first == 0) continue; // Client was removed

        Client *server = pair.second;
        if (!FD_ISSET(server->sock, readSockets)) continue; // Nothing from socket

        if (recv(server->sock, buffer, buffersize, MSG_DONTWAIT) == 0) { // Socket left
            print(server->name + " CLOSED CONNECTION:" + std::to_string(server->sock));
            removeStack.push(server->sock);
            continue;
        }
        serverCommand(server->sock, buffer, buffersize);  
    }
}

/**
 * Loops through all clients and checks if they are set (have updated i.e. sent message, disconnected), 
 * and removes them or excecutes their command accordingly
 * 
 * @param buffer Character array sent from client
 * @param buffersize Size of the character array sent from client
 * @param openSockets fd_set of all connected sockets
 * @param readSockets fd_set of connected sockets, not including newly connected
 * @param maxfds The highest file descriptor we have open
 * 
 */
void checkClientCommands(char *buffer, int buffersize, fd_set *openSockets, fd_set *readSockets, int *maxfds) {
    for(auto const& pair : clients) {
        if (pair.first == 0) continue; // Client was removed

        Client *client = pair.second; 
        if (!FD_ISSET(client->sock, readSockets)) continue; // Nothing from socket

        if (recv(client->sock, buffer, buffersize, MSG_DONTWAIT) == 0) { // Socket left
            print(client->name + " CLOSED CONNECTION:" + std::to_string(client->sock));
            removeStack.push(client->sock);
            continue;
        } 
        clientCommand(client->sock, openSockets, maxfds, buffer, buffersize); 
    }
}

/// A function that sends keepalive messages periodically once per minute
void updateServer(){
    int kickIdle = 0;
    int kickIdleInterval = 3;
    while (!FINISHED) {
        std::this_thread::sleep_for(std::chrono::minutes(1));
        for (auto const& pair : servers) {
            // Send to all 1 hop servers that there are incomming messages
            if (messages.count(pair.second->name)) {
                sendKeepAlive(pair.second->sock, messages[pair.second->name].size());
            } else {
                sendKeepAlive(pair.second->sock, 0);
            }
        }
        kickIdle++;
        if (kickIdle == kickIdleInterval) {
            for (auto const & server : idleServers) {
                removeStack.push(server);
            }
            idleServers.clear();
            for (auto const &pair : servers) {
                idleServers.push_back(pair.second->sock);
            }
        }
        kickIdle = kickIdle % kickIdleInterval;
        // printf("kickIdle is at:%d\n",kickIdle);
    }
}

int main(int argc, char* argv[]) {
    char buffer[5000];              // buffer for reading from servers

    int listenServerSock;           // Socket for server connections to server
    int listenClientSock;           // Socket for client connections to server
    int serverSock;                 // Socket of connecting servers
    int clientSock;                 // Socket of connecting clients
    
    fd_set openSockets;             // Current open server sockets 
    fd_set readSockets;             // Socket list for select()  
    fd_set exceptSockets;           // Exception server socket list
    
    int maxfds;                     // Passed to server select() as max fd in set
    sockaddr_in client_addr;
    socklen_t clientLen;
    
    if (argc != 2) {
        if (argc != 3) {
            printf("Usage: chat_server <ip port servers>\n");
            exit(0);
        } else {
            HOST_NAME = argv[2];
        }
    }

    HOST_IP = get_ip();
    HOST_PORT = atoi(argv[1]);
    int clientPort = HOST_PORT+1;

    // Setup sockets to listen to servers
    listenServerSock = open_socket(HOST_PORT);   // Listen to servers
    print("SERVER LISTENING ON PORT: " + std::to_string(HOST_PORT));

    if (listen(listenServerSock, BACKLOG) < 0) {
        print("LISTEN FAILED ON PORT: " + std::to_string(HOST_PORT));
        exit(0);
    }

    // Setup socket to listen to clients
    listenClientSock = open_socket(clientPort);  // Listen to clients

    if (listen(listenClientSock, BACKLOG) < 0) {
        print("LISTEN FAILED ON PORT: " + std::to_string(clientPort));
        exit(0);
    }
    
    FD_ZERO(&openSockets);
    FD_SET(listenServerSock, &openSockets);
    FD_SET(listenClientSock, &openSockets);
    maxfds = std::max(listenServerSock, listenClientSock);

    std::thread getUpdates(updateServer);

    while(!FINISHED) {
        
        // Get modifiable copy of readServerSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        // Look at sockets and see which ones have something to be read()
        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if (n < 0){
            print("SELECT FAILED - SHUTTING DOWN");
            FINISHED = true;
        } else {
            // First, accept  any new client connections to the server
            if (FD_ISSET(listenClientSock, &readSockets)) {
                clientSock = accept(listenClientSock, (struct sockaddr *) &client_addr, &clientLen);
                char passcode[20];
                timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 100 * 1000; // 100 msec
                setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(tv));
                recv(clientSock, passcode, sizeof(passcode), 0); // this is only enough too keep mindless processies of our server

                if (clients.size() < MAX_HOSTS && clientSock > 0 && std::string(passcode).compare("password123") == 0) {
                    print("SERVER ACCEPT'S CLIENT ON SOCKET " + std::to_string(clientSock));
                    // Add new client to the list of open sockets
                    FD_SET(clientSock, &openSockets);

                    // And update the maximum file descriptor
                    maxfds = std::max(maxfds, clientSock);

                    // create a new client to store information.
                    clients[clientSock] = new Client(clientSock);
                    clients[clientSock]->name = std::string("CLIENT:") + std::to_string(clientSock);

                } else {
                    std::string unknownClient = "Poopy-di scoop Scoop-diddy-whoop Whoop-di-scoop-di-poop "
                                                "Poop-di-scoopty Scoopty-whoop Whoopity-scoop whoop-poop "
                                                "Poop-diddy whoop-scoop Poop poop Scoop-diddy-whoop Whoop"
                                                "-diddy-scoop Whoop-diddy-scoop poop. Client for tsamgroup112.";
                    
                    send(clientSock, unknownClient.c_str(), unknownClient.length(), 0);
                    close(clientSock);
                    if (maxfds == clientSock) {
                        for(auto const& p : clients) {
                            maxfds = std::max(maxfds, p.second->sock);
                        }
                    }
                    // And remove from the list of open sockets.
                    FD_CLR(clientSock, &openSockets);
                    print("SERVER DOES NOT ACCEPT'S CLIENT ON SOCKET: UNABLE TO SELECT");
                }
                n--; // Decrement the number of sockets waiting to be dealt with
            }
            // Accept any new server connections to the server

            if (FD_ISSET(listenServerSock, &readSockets)) {
                serverSock = accept(listenServerSock, (struct sockaddr *) &client_addr, &clientLen);
            
                if (servers.size() < MAX_HOSTS && serverSock > 0) {
                    print("SERVER ACCEPT'S SERVER ON SOCKET " + std::to_string(serverSock));
                    // Add new client to the list of open sockets
                    FD_SET(serverSock, &openSockets);

                    // And update the maximum file descriptor
                    maxfds = std::max(maxfds, serverSock);

                    // create a new client to store information.
                    servers[serverSock] = new Client(serverSock);

                } else {
                    close(serverSock);
                    if (maxfds == serverSock) {
                        for(auto const& p : servers) {
                            maxfds = std::max(maxfds, p.second->sock);
                        }
                    }
                    // And remove from the list of open sockets.
                    FD_CLR(serverSock, &openSockets);
                    std::string log = "SERVER DOES NOT ACCEPT'S SERVER ON SOCKET";
                    if (serverSock > 0) {
                        log += " " + std::to_string(serverSock) + ": TO MANY CONNECTIONS";
                    } else {
                        log += ": UNABLE TO SELECT";
                    }
                    print(log);
                }
                n--; // Decrement the number of sockets waiting to be dealt with
            }
            while(n-- > 0) {
                // Now check for commands from servers and clients
                checkServerCommands(buffer, sizeof(buffer), &readSockets);
                checkClientCommands(buffer, sizeof(buffer), &openSockets, &readSockets, &maxfds);
                while (!removeStack.empty()) {
                    closeConnection(removeStack.top(), &openSockets, &maxfds);
                    removeStack.pop();
                }
            }
        }
    }
    
    for(auto const& pair : servers) {
        std::string goodbye = "*LEAVE," + pair.second->ip + "," + std::to_string(pair.second->port) + "#";
        send(pair.second->sock, goodbye.c_str(), goodbye.length(), 0);
    }
    
    for (auto const& pair : clients) {
        std::string goodbye = "Goodbye";
        send(pair.second->sock, goodbye.c_str(), goodbye.length(), 0);
        closeConnection(pair.second->sock, &openSockets, &maxfds);
    }
    
    getUpdates.join(); // Join update thread
    
    print("SERVER IS SHUTTING DOWN");
}

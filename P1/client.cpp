
// Simple client for TSAM-409 Assignment 1
//
// Compile: g++ -Wall -std=c++11 client.cpp -o client 
//
// Command line: ./client <ip address> <ip port>
//
// Author: Ymir Thorleifsson (ymir19@ru.is)

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include <iostream>

int sock;
char buffer[4096];

int establish_connection(char ip_addr[], char port[]) {
    
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("%n",&sock);
        perror("Failed to make socket");
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(port));

    if (inet_pton(AF_INET, ip_addr, &serv_addr.sin_addr) <= 0) {
        perror("Failed to set socket address");
        return -1;
    }
    
    if(connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Failed to connect to server");
        return -1;
    }
    
    printf("Connection to server has been established\n");

    return 0;
}


int main(int argc, char* argv[]) {
    
    if(argc != 3) {
        printf("Usage: client <ip address> <ip port>\n");
        exit(0);
    }

    establish_connection(argv[1], argv[2]);


    char cmd[32] = "SYS ls";
    send(sock, cmd, sizeof(cmd), 0);

}
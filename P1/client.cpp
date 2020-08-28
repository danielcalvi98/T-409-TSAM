
// Simple client for TSAM-409 Assignment 1
//
// Compile: g++ -Wall -std=c++11 client.cpp -o client 
//
// Command line: ./client <ip address> <ip port>
//
// Author: Ymir Thorleifsson (ymir19@ru.is)

// TODO
// 1 Socket
// 2 connect
// 3 Send/recv
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

int sockfd;
char buffer[4096];
// char ip_addr[] = "192.168.1.34";

int establish_connection(char ip_addr[], char port[]) {
    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("Failed to make socket");
        return -1;
    }

    // Setup socket address structue for connection
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(port));

    // convert string ip-address to a network address
    if (inet_pton(AF_INET, ip_addr, &serv_addr.sin_addr) < 1) {
        perror("Failed to set socket address");
        return -1;
    }

    // connect to server
    if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Failed to connect to server");
        return -1;
    }

    return 0;
}


int main(int argc, char* argv[]) {
    if(argc != 3)
    {
        printf("Usage: client <ip address> <ip port>\n");
        exit(0);
    }
    // printf("You managed to type in the ip address: %s and the port: %s WOW!\n", argv[1], argv[2]);
    establish_connection(argv[1], argv[2]);
}
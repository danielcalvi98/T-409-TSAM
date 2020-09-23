/* 
=====================================
            PORT SCANNER
=====================================
              Authors:
        Kristofer Robertsson
          Ymir Thorleifsson
=====================================
  - 60% of the time it works
    every time 
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <stdio.h>
#include <iostream>
#include <fstream>

// #include "scanner_utils.h"

int send_to_server(char* buffer, int bufferlen, char* message, int socket, sockaddr_in destaddr, int port) {
    bzero(buffer, sizeof(buffer)); // Clear buffer

    /* Set server port */
    destaddr.sin_port = htons(port);

    /* Write messsage to buffer */
    int length = strlen(message) + 1;

    /* Send message to buffer */
    if (sendto(socket, message, length, 0x0, (const struct sockaddr *) &destaddr, sizeof(destaddr)) < 0) {
        return -1;
    }

    /* Get response from server */
    bzero(buffer, sizeof(buffer)); // Clear buffer
    length = recvfrom(socket, buffer, bufferlen, 0x0, NULL, NULL);
    if (length > 0) {
        return length;
    } else {
        return 0;
    }
}

int scan_range(int socket, int low, int high, char* message, sockaddr_in destaddr, char* buffer, int* ports_found_list) {
    /* Keeps track of ports found */
    int ports_found = 0;
    
    /* Loops through ports */
    for (int port = low; port <= high; port++) {

        int length = send_to_server(buffer, sizeof(buffer), message, socket, destaddr, port);
        if (length > 0) {
            ports_found_list[ports_found] = port;
            ports_found++;
        } 
    }
    return ports_found;
}

int main(int argc, char *argv []) {

    if(argc != 4) {
        printf("Usage: ./scanner <IP address>" 
               " <low port> <high port>\n");
        exit(0);
    }
    /* Define variables */
    char* address = argv[1];                             
    int low       = atoi(argv[2]);
    int high      = atoi(argv[3]);  

    int     udp_sock;
    char    buffer[1024];
    int     known_ports = 4;
    int     open_ports[high - low + 1];
    int     ports_found = 0;
    int     length = 0;


    struct sockaddr_in  destaddr;
    struct timeval      timeout;


    /* Create socket */
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Unable to open socket");
        return -1;
    }
    /* Timeout on socket */
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));

    /* Server Address */
    destaddr.sin_family = AF_INET;
    inet_aton(address, &destaddr.sin_addr);

    /* Scans range of ports */
    ports_found = scan_range(udp_sock, low, high, "knock", destaddr, buffer, open_ports); // in headerfile

    printf("Found %d open ports in range %d-%d: \n", ports_found, low, high);
    for (int port = 0; port < ports_found; port++) {
        printf("%d ", open_ports[port]);
        if ((port + 1) == ports_found) {
            printf("\n");
        }
    }
    return 1;
}
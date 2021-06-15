/* 
=====================================
|||         PORT SCANNER          |||
=====================================
|||   60% of the time it works    |||
|||         every time            |||
=====================================
|||           Authors:            |||
|||     Kℜistōfeℜ ℜobeℜtsson     |||
|||       Ύmiℜ ƥōℜleif⒮⒮on      |||
=====================================
✐
*/

#include <sys/socket.h>     // Sockets
#include <netinet/in.h>     // Address'
#include <arpa/inet.h>      // Inet_addr
#include <string.h>         // Memset, strcpy
#include <stdio.h>          // Printf
#include <iostream>         // Atoi, exit
#include "scanner_utils.h"  // Our utilty module


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
    int     open_ports[high - low + 1];
    int     ports_found = 0;

    struct sockaddr_in  destaddr;
    struct timeval      timeout;

    /*
    /////////////////\\\\\\\\\\\\\\\\\\\
    |||           PHASE 1            |||
    \\\\\\\\\\\\\\\\\///////////////////
    */

    /* Create socket */
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Unable to open socket");
        return -1;
    }
    /* Timeout on socket */
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));

    /* Server Address */
    destaddr.sin_family = AF_INET;
    inet_aton(address, &destaddr.sin_addr);

    /* Scans range of ports */
    ports_found = scan_range(udp_sock, low, high, (char *) "knock", destaddr, buffer, open_ports); // in headerfile

    printf("Found %d open ports in range %d-%d: \n", ports_found, low, high);
    for (int port = 0; port < ports_found; port++) {
        printf("%d ", open_ports[port]);
        if ((port + 1) == ports_found) {
            printf("\n");
        }
    }
    return 1;
}
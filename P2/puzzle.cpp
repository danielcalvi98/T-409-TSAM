/* 
========================================================================
                                 PUZZLE
========================================================================
                                 Authors:
                           Kristofer Robertsson
                            Ymir Thorleifsson
========================================================================
Messages we got:

PORT 4042:

I am the oracle, send me a comma-seperated list of the hidden ports, 
and I shall show you the way.

PORT 4097:  

The dark side of network programming is a pathway to many abilities 
some consider to be...unnatural. (https://en.wikipedia.org/wiki/Evil_bit)
Send us a message containing your group number in the form "$group_#$" 
where # is your group number.

PORT 4099:  

My boss told me not to tell anyone that my secret port is 4002

*/

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include <iostream>
#include <string.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <fstream>

int main(int argc, char* argv[]) {
    if(argc != 1) {
        printf("Usage: ./puzzle \n");
        exit(0);
    }

    printf("Scanning ports...\n");
    system("./scanner 130.208.243.61 4000 4100");

    int     udp_sock;   
    char    buffer[1024];
    int     length;                       

    struct sockaddr_in  destaddr;
    struct timeval      timeout;

    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Failed to make socket");
        close(udp_sock);
        return -1;
    }

    timeout.tv_sec = 0;
    timeout.tv_usec = 10;
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));


    destaddr.sin_family = AF_INET;
    inet_aton("130.208.243.61", &destaddr.sin_addr);

    int port;
    std::ifstream openports("open_ports.txt");

    while (openports >> port) {

        destaddr.sin_port = htons(port);

        strcpy(buffer, "knock");
        length = strlen(buffer) + 1;


        if (sendto(udp_sock, buffer, length, 0x0, (struct sockaddr *)&destaddr, sizeof(destaddr)) < 0) {
            perror("failed to send");
        }
        
        bzero(buffer, length);

        int msg_len = recvfrom(udp_sock, buffer, sizeof(buffer), 0x0, NULL, NULL);

        if (msg_len < 0) {
            printf("No message\n");
        } else {
        printf("Port %d: %s\n", port, buffer);

        bzero(buffer, sizeof(buffer));

        }
    }
    close(udp_sock);
    return 1;
}
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

    int sock;
    struct timeval tv;
    
    char buffer[1024];                                 
    char msg[16];
    strcpy(msg, "knock");

    std::ifstream openports("open_ports.txt");
    
    int port;

    while (openports >> port) {

        if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
            perror("Failed to make socket");
            close(sock);
            exit(-1);
        }

        struct sockaddr_in server_address;
        server_address.sin_family       = AF_INET;
        server_address.sin_port         = htons(port);
        server_address.sin_addr.s_addr  = inet_addr("130.208.243.61"); //skel.ru.is

        if (sendto(sock, msg, sizeof(msg), 0x0, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            perror("failed to send");
            exit(-1);
        }

        
        tv.tv_sec = 1;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        int msg_len = recvfrom(sock, buffer, sizeof(buffer), 0x0, NULL, NULL);

        if (msg_len < 0) {
            printf("Nothing\n");
            close(sock);
            continue;
        } 

        printf("Port %d: %s", port, buffer);
        for (int i = 0; i < (int) sizeof(buffer); i++) {
            buffer[i] = '\0';
        }
        close(sock);
        }
    printf("\n");
}
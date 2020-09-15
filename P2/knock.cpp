/* 
========================================================================
                                 KNOCK
========================================================================
                                 Authors:
                           Kristofer Robertsson
                            Ymir Thorleifsson
========================================================================
message we got

Port 32685: Hello, group_42! To get the secret phrase, 
send me a message with a valid UDP checksum of 0x374f!�

Port 0: Hello, group_42! To get the secret phrase, 
send me a message with a valid UDP checksum of 0x92d1!

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

#define PORT 4098
#define IP "130.208.243.61"

int main(int argc, char* argv[]) {
    if(argc != 1) {
        printf("Usage: ./puzzle \n");
        exit(0);
    }

    int sock;
    struct timeval tv;
    
    char buffer[1024];                                 
    char msg[16];
    strcpy(msg, "$group_42$");
    strcpy(msg, "$group_42$");

    std::ifstream openports("open_ports.txt");
    
    int port;

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Failed to make socket");
        close(sock);
        exit(-1);
    }

    struct sockaddr_in server_address;
    server_address.sin_family       = AF_INET;
    server_address.sin_port         = htons(PORT);
    server_address.sin_addr.s_addr  = inet_addr(IP); //skel.ru.is

    if (sendto(sock, msg, sizeof(msg), 0x0, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("failed to send");
        exit(-1);
    }

    
    tv.tv_sec = 1;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int msg_len = recvfrom(sock, buffer, sizeof(buffer), 0x0, NULL, NULL);

   
    printf("Port %d: %s", port, buffer);
    for (int i = 0; i < (int) sizeof(buffer); i++) {
        buffer[i] = '\0';
    }

    close(sock);
    printf("\n");
}
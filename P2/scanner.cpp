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


int main(int argc, char* argv[]) {
    if(argc != 4) {
        printf("Usage: ./scanner <IP address>" 
               " <low port> <high port>\n");
        exit(0);
    }
    /* Inputs */
    char* ip_addr = argv[1];                             
    int port_low;   sscanf(argv[2], "%d", &port_low);   
    int port_high;  sscanf(argv[3], "%d", &port_high);   
    
    int sock;
    struct timeval tv;

    char buffer[1024];                                 
    char msg[16];
    strcpy(msg, "knock");
    
    printf("Port scanning IP %s, from ports %d to %d\n", 
            ip_addr, port_low, port_high);

    for (int port = port_low; port <= port_high; port++) {

        printf("Sending UDP to port %d: ", port);


        if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
            perror("Failed to make socket");
            close(sock);
            exit(-1);
        }

        struct sockaddr_in server_address;
        server_address.sin_family       = AF_INET;
        server_address.sin_port         = htons(port);
        server_address.sin_addr.s_addr  = inet_addr(ip_addr);

        if (sendto(sock, msg, sizeof(msg), 0x0, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            perror("failed to send");
            exit(-1);
        }

        
        tv.tv_sec = 1;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        if (recvfrom(sock, buffer, sizeof(buffer), 0x0, NULL, NULL) < 0) {
            printf("CLOSED\n");
            close(sock);
            continue;
        } 

        printf("OPEN\n");
        // printf("%s", buffer);
        for (int i = 0; i < (int) sizeof(buffer); i++) {
            buffer[i] = '\0';
        }
        close(sock);
        }
    printf("\n");
}
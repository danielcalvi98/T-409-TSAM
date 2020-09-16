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

#include <fstream>


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
    
    printf("Port scanning IP %s, from ports %d to %d\n", 
            ip_addr, port_low, port_high);

    int sock;
    struct timeval tv;

    char buffer[1024];                                 
    char msg[16];
    strcpy(msg, "knock");
    
    std::ofstream openports;
    // openports.open("open_ports.txt");

    for (int port = port_low; port <= port_high; port++) {

        printf("Sending UDP package to port %d: ", port);


        if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
            perror("Failed to make socket");
            close(sock);
            exit(-1);
        }
        tv.tv_sec = 1;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        struct sockaddr_in server_address;
        server_address.sin_family       = AF_INET;
        server_address.sin_port         = htons(port);
        server_address.sin_addr.s_addr  = inet_addr(ip_addr);

        if (sendto(sock, msg, sizeof(msg), 0x0, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            perror("failed to send");
            exit(-1);
        }

        
        int msg_len = recvfrom(sock, buffer, sizeof(buffer), 0x0, NULL, NULL);

        if (msg_len <= 0) {
            printf("CLOSED\n");
        } else {
            // openports << port << std::endl;
            printf("OPEN\n");
        }
        close(sock);
    }
    // openports.close();
    return 0;
}
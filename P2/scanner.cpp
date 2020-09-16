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
#include <fstream>

int main(int argc, char *argv []) {

    if(argc != 4) {
        printf("Usage: ./scanner <IP address>" 
               " <low port> <high port>\n");
        exit(0);
    }

    char* address = argv[1];                             
    int low  = atoi(argv[2]);
    int high = atoi(argv[3]);  
    // int low  = 4000;
    // int high = 4100;

    int udp_sock;
    char buffer[1024];
    int open_ports[high - low + 1];
    int ports_found = 0;
    int length = 0;
    int checks = 3;
    int check_nr = 0;
    struct sockaddr_in destaddr;
    struct timeval timeout;


    /*  */
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Unable to open socket");
        return(-1);
    }
    /*  */
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));

    /*  */
    destaddr.sin_family = AF_INET;
    inet_aton(address, &destaddr.sin_addr);
    // inet_aton("130.208.243.61", &destaddr.sin_addr);

    while (check_nr < checks) {
        check_nr++;
        for (int port = low; port <= high; port++) {
            /*  */
            destaddr.sin_port = htons(port);

            /*  */
            strcpy(buffer, "knock");
            length = strlen(buffer) + 1;

            /*  */
            if (sendto(udp_sock, buffer, length, 0x0, (const struct sockaddr *) &destaddr, sizeof(destaddr)) < 0) {
                printf("Failed to send to port %d", port);
                perror("");
            }

            bzero(buffer, length); //
            
            /*  */
            if (length = recvfrom(udp_sock, buffer, sizeof(buffer), 0x0, NULL, NULL) > 0) {
                // printf("Port %d: OPEN\n", port);
                bool new_port = true;
                for (int i = 0; i < ports_found; i++) {
                    if (port == open_ports[i]) {
                        new_port = false;
                        break;
                    }
                }
                if (new_port) {
                    open_ports[ports_found] = port;
                    ports_found++;
                    check_nr = 0;
                }
                bzero(buffer, sizeof(buffer)); //
            }
        }
    }

    /*  */
    std::ofstream openports;
    openports.open("open_ports.txt");

    printf("Found %d open ports in range %d-%d: \n", ports_found, low, high);
    for (int port = 0; port < ports_found; port++) {
        openports << open_ports[port] << std::endl;
        printf("%d ", open_ports[port]);
        if ((port + 1) == ports_found) {
            printf("\n");
        }
    }
    
    // close(udp_sock);
    openports.close();
    return(1);
}
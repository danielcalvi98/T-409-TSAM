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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

#include <stdio.h>
#include <fstream>

int main(int argc, char* argv[]) {
    if(argc != 1) {
        printf("Usage: ./puzzle \n");
        exit(0);
    }
    char* address = "130.208.243.61";
    int low =  4000;
    int high = 4100;
    std::string command;

    printf("Running port scanner: ");

    command += "./scanner ";
    command += address;
    command += " ";
    command += std::to_string(low);
    command += " ";
    command += std::to_string(high);
    puts(command.c_str());
    system(command.c_str());
    printf("\nMessages:\n");

    int udp_sock;
    char buffer[1024];
    int length;
    int port;

    struct sockaddr_in destaddr;
    struct timeval timeout;
    std::ifstream openports("open_ports.txt");


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

    while (openports >> port) {
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
            printf("Port %d:\n%s\n\n", port, buffer);
            bzero(buffer, sizeof(buffer)); //
        }
    }
}

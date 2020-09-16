#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#define BUFFERSIZE 1024

int main(int argc, char *argv []) {

    int udp_sock;
    int low  = 4000;
    int high = 4100;
    char buffer[BUFFERSIZE];
    int length;
    struct sockaddr_in destaddr;
    struct timeval timeout;

    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Unable to open socket");
        return(-1);
    }

    timeout.tv_sec = 0;
    timeout.tv_usec = 200;
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));

    destaddr.sin_family = AF_INET;
    inet_aton("130.208.243.61", &destaddr.sin_addr);

    for (int port = low; port <= high; port++) {
        destaddr.sin_port = htons(port);

        strcpy(buffer, "knock");
        length = strlen(buffer) + 1;

        if (sendto(udp_sock, buffer, length, 0x0, (const struct sockaddr *) &destaddr, sizeof(destaddr)) < 0) {
            printf("Failed to send to port %d", port);
            perror("");
        }
        bzero(buffer, length);
        if (length = recvfrom(udp_sock, buffer, BUFFERSIZE, 0x0, NULL, NULL) > 0) {
            printf("Port %d: OPEN\n", port);
            bzero(buffer, length);
        } else {
            printf("Port %d: CLOSED\n", port);
        }
    }
    return(1);
}
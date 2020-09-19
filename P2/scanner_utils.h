#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>

int scan_range(int socket, int low, int high, sockaddr_in destaddr, char* buffer, int* ports_found_list) {
    int ports_found = 0;
    
    for (int port = low; port <= high; port++) {
        /* Server port */
        destaddr.sin_port = htons(port);

        bzero(buffer, sizeof(buffer)); // Clear buffer
        /* Message */
        strcpy(buffer, "knock");
        int length = strlen((char*) &buffer) + 1;

        /* Send to server */
        sendto(socket, buffer, length, 0x0, (const struct sockaddr *) &destaddr, sizeof(destaddr));
            
        bzero(buffer, length); // Clear buffer
        
        /* Recive from server */
        length = recvfrom(socket, buffer, sizeof(buffer), 0x0, NULL, NULL);
        if (length > 0) {
            ports_found_list[ports_found] = port;
            ports_found++;
        }
    }
    return ports_found;
}
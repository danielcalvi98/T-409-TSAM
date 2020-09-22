#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>

int send_to_server(char* buffer, int bufferlen, char* message, int socket, sockaddr_in destaddr, int port) {
    bzero(buffer, sizeof(buffer)); // Clear buffer

    /* Set server port */
    destaddr.sin_port = htons(port);

    /* Write messsage to buffer */
    int length = strlen(message) + 1;

    /* Send message to buffer */
    if (sendto(socket, message, length, 0x0, (const struct sockaddr *) &destaddr, sizeof(destaddr)) < 0) {
        return -1;
    }

    /* Get response from server */
    bzero(buffer, sizeof(buffer)); // Clear buffer
    length = recvfrom(socket, buffer, bufferlen, 0x0, NULL, NULL);
    if (length > 0) {
        return length;
    } else {
        return 0;
    }
}

int scan_range(int socket, int low, int high, char* message, sockaddr_in destaddr, char* buffer, int* ports_found_list) {
    /* Keeps track of ports found */
    int ports_found = 0;
    
    /* Loops through ports */
    for (int port = low; port <= high; port++) {

        int length = send_to_server(buffer, sizeof(buffer), message, socket, destaddr, port);
        if (length > 0) {
            ports_found_list[ports_found] = port;
            ports_found++;
        } 
    }
    return ports_found;
}

u_short csum(u_short* buffer, int size) {
    register long   checksum = 0;

    while (size > 1) {
        checksum += *buffer++;
        size -= sizeof(u_short);
    }
    if (size) {
        checksum =* (u_char*) buffer;
    }
    checksum = (checksum >> 16) + (checksum & 0xFFFF);
    
    checksum = checksum + (checksum >> 16);

    return (u_short) (~checksum);
}

struct pseudo_header {
    u_int32_t source;
    u_int32_t dest;
    u_int8_t  zeros;
    u_int8_t  protocol;
    u_int16_t udp_length;
};

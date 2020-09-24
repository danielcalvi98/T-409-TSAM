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

// int scan_range(int socket, int low, int high, char* message, sockaddr_in destaddr, char* buffer, int* ports_found_list) {
//     /* Keeps track of ports found */
//     int ports_found = 0;
    
//     /* Loops through ports */
//     for (int port = low; port <= high; port++) {

//         int length = send_to_server(buffer, sizeof(buffer), message, socket, destaddr, port);
//         if (length > 0) {
//             ports_found_list[ports_found] = port;
//             ports_found++;
//         } 
//     }
//     return ports_found;
// }

// u_short csum(u_short* buffer, int size) {
//     register long   checksum = 0;

//     while (size > 1) {
//         checksum += *buffer++;
//         size -= sizeof(u_short);
//     }
//     if (size) {
//         checksum =* (u_char*) buffer;
//     }
//     checksum = (checksum >> 16) + (checksum & 0xFFFF);
    
//     checksum = checksum + (checksum >> 16);

//     return (u_short) (~checksum);
// }

struct pseudo_header {
    u_int32_t source;
    u_int32_t dest;
    u_int8_t  zeros;
    u_int8_t  protocol;
    u_int16_t udp_length;
};

u_short csum(u_short *ptr,int nbytes) {
    register long sum;
    u_short oddbyte;
    register short answer;

    sum = 0;
    while(nbytes>1) {
        sum += *ptr++;
        nbytes -= 2;
    }
    if(nbytes == 1) {
        oddbyte = 0;
        *((u_char*) &oddbyte) =* (u_char*) ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
    answer = (short) ~sum;

    return answer;
}

void DumpHex(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}

int scan_range(int socket, int low, int high, char* message, sockaddr_in destaddr, 
                                                char* buffer, int* ports_found_list) {
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

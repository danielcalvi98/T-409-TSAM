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
#include <iostream>
#include <fstream>

int send_to_server(char* buffer, int bufferlen, char* message, int socket, sockaddr_in servaddr, int port) {
    /* Set server port */
    servaddr.sin_port = htons(port);

    /* Write messsage to buffer */
    int length = strlen(message) + 1;

    /* Send message to buffer */
    if (sendto(socket, message, length, 0x0, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        return -1;
    }

    /* Get response from server */
    bzero(buffer, sizeof(buffer)); // Clear buffer
    length = recvfrom(socket, buffer, bufferlen, 0x0, NULL, NULL);
    if (length > 0) {
        return 1;
    } else {
        return 0;
    }
}


int main(int argc, char* argv[]) {
    if(argc != 1) {
        printf("Usage: ./puzzle \n");
        exit(0);
    }
    /* Define variables */
    char*       address = "130.208.243.61";
    int         low =  4000;
    int         high = 4100;
    std::string command;
    /* Create command */
    command += "./scanner ";
    command += address;
    command += " ";
    command += std::to_string(low);
    command += " ";
    command += std::to_string(high);
    puts(command.c_str());

    int     udp_sock;
    char    buffer[1024];
    int     port;
    int     known_ports = 4;
    int     ports_found = 0;
    char    write_to_file[known_ports][1024];
    int     tries = 2;

    struct sockaddr_in  destaddr;
    struct timeval      timeout;


    /* Create socket */
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Unable to open socket");
        return(-1);
    }

    /* Set timeout to one micro second */
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));

    /* Set server address */
    destaddr.sin_family = AF_INET;
    inet_aton(address, &destaddr.sin_addr);

    while (ports_found < known_ports) {
        ports_found = 0;

        /* Clear write to file array */
        for (int i = 0; i < known_ports; i++) {
            bzero(write_to_file[i], sizeof(write_to_file[i]));
        }
        /* Get ports */
        printf("Running port scanner:\n");
        system(command.c_str());
        // printf("\nMessages:\n");
        std::ifstream openports("open_ports.txt");
        bool stop = false;
        while (openports >> port) {
            if (stop) {
                break;
            }
            for (int i = 0; i < tries; i++) {
                int status = send_to_server(buffer, sizeof(buffer), "knock", udp_sock, destaddr, port);
                if (status > 0) {
                    std::string message;
                    message = std::to_string(port);
                    message += ": ";
                    message += buffer;
                    strcpy(write_to_file[ports_found], message.c_str());
                    bzero(buffer, sizeof(buffer));
                    ports_found++;

                    break;
                } 
                stop = true;
                break;
            }
        }
        printf("Ports found %d/%d\n", ports_found, known_ports);
        if (ports_found < known_ports) {
            printf("Trying again...\n");
        }
    }
    
    /* Save to file */
    std::ofstream messages;
    messages.open("ports_messages.txt");
    printf("Messages:\n");
    for (int message = 0; message < known_ports; message++) {
        messages << write_to_file[message] << std::endl;
        // printf("%s\n", write_to_file[message], sizeof(write_to_file[message]));
        std::string the_message(write_to_file[message], sizeof(write_to_file[message]));
        size_t space_pos = the_message.find(" ");

        std::string p;
        
        if (space_pos != std::string::npos) {
            p = the_message.substr(0, 4);
            the_message = the_message.substr(space_pos + 1);
        }
        int port = atoi(p.c_str()); 
        printf("%s\n", the_message.c_str());
        if (the_message.find("$group_#$")) {
            printf("Sending '$group_42$', to port %d\n",port);
            int status = send_to_server(buffer, sizeof(buffer), "$group_42$", udp_sock, destaddr, port);
            printf(">%s\n", buffer);

        }

    }

    messages.close();
}
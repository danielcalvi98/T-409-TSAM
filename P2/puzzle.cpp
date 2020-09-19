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

#include <scanner.cpp>

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
    char    messages[known_ports][1024];
    int     tries = 1;

    struct sockaddr_in  destaddr;
    struct timeval      timeout;


    /* Create socket */
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Unable to open socket");
        return(-1);
    }

    /* Set timeout to one micro second */
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));

    /* Set server address */
    destaddr.sin_family = AF_INET;
    inet_aton(address, &destaddr.sin_addr);

    /* Get messages from ports */
    // while (ports_found < known_ports) {
        ports_found = 0;

        /* Clear write to file array */
        for (int i = 0; i < known_ports; i++) {
            bzero(messages[i], sizeof(messages[i]));
        }
        /* Get ports */
        printf("Running port scanner:\n");
        system(command.c_str());
        // printf("\nMessages:\n");
        std::ifstream openports("open_ports.txt");
        bool stop = false;
        while (openports >> port) {
            if (stop) {
                break; // Quit if no message was got
            }
            /* Send a few tries */
            for (int i = 0; i < tries; i++) {
                int status = send_to_server(buffer, sizeof(buffer), "knock", udp_sock, destaddr, port);
                if (status > 0) {
                    std::string message;
                    message = std::to_string(port);
                    message += ": ";
                    message += buffer;
                    strcpy(messages[ports_found], message.c_str());
                    bzero(buffer, sizeof(buffer));
                    ports_found++;

                    break; // Continue with next port
                } 
                stop = true; // Ports have changed so we move on
                break;
            }
        }
        printf("Ports found %d/%d\n", ports_found, known_ports);
        if (ports_found < known_ports) {
            printf("Trying again...\n");
        }
    // }
    
    /* Save to file */
    std::ofstream messages_file;
    messages_file.open("ports_messages.txt");
    printf("Messages:\n");
    for (int message = 0; message < known_ports; message++) {
        messages_file << messages[message] << std::endl;

        std::string the_message(messages[message], sizeof(messages[message]));

        port        = atoi(the_message.substr(0, 4).c_str());
        the_message = the_message.substr(6);

        printf("%d %s\n", port, the_message.c_str());
        if (the_message.find("$group_#$") != std::string::npos) {
            printf("Sending '$group_42$', to port %d\n",port);
            int status = send_to_server(buffer, sizeof(buffer), "$group_42$", udp_sock, destaddr, port);
            printf(">%s\n", buffer);

        }

    }

    messages_file.close();
}


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <stdio.h>
#include <iostream>
#include <fstream>

int main(int argc, char *argv []) {

    if(argc != 4) {
        printf("Usage: ./scanner <IP address>" 
               " <low port> <high port>\n");
        exit(0);
    }

    /* Define variables */
    char* address = argv[1];                             
    int low       = atoi(argv[2]);
    int high      = atoi(argv[3]);  

    int     udp_sock;
    char    buffer[1024];
    int     known_ports = 4;
    int     open_ports[high - low + 1];
    int     ports_found = 0;
    int     length = 0;


    struct sockaddr_in  destaddr;
    struct timeval      timeout;


    /* Create socket */
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Unable to open socket");
        return(-1);
    }
    /* Timeout on socket */
    timeout.tv_sec = 0;
    timeout.tv_usec = 10;
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));

    /* Server Address */
    destaddr.sin_family = AF_INET;
    inet_aton(address, &destaddr.sin_addr);
    // inet_aton("130.208.243.61", &destaddr.sin_addr);
    
    // while (ports_found < known_ports) {
    for (int port = low; port <= high; port++) {
        /* Server port */
        destaddr.sin_port = htons(port);

        /* Message */
        strcpy(buffer, "knock");
        length = strlen(buffer) + 1;

        /* Send to server */
        if (sendto(udp_sock, buffer, length, 0x0, (const struct sockaddr *) &destaddr, sizeof(destaddr)) < 0) {
            printf("Failed to send to port %d", port);
            perror("");
        }

        bzero(buffer, length); // Clear buffer
        
        /* Recive from server */
        length = recvfrom(udp_sock, buffer, sizeof(buffer), 0x0, NULL, NULL);
        if (length > 0) {
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
                // printf(".");
            }
            bzero(buffer, sizeof(buffer)); //
        }
    }
    // printf("\n");
    // if (ports_found != known_ports) {
    //     for (int i = 0; i < known_ports; i++) {
    //         open_ports[i] = 0;
    //     }
    //     ports_found = 0;
    // }
    // }

    // /* Save to file */
    // std::ofstream openports;
    // openports.open("open_ports.txt");

    printf("Found %d open ports in range %d-%d: \n", ports_found, low, high);
    for (int port = 0; port < ports_found; port++) {
    //     openports << open_ports[port] << std::endl;
        printf("%d ", open_ports[port]);
        if ((port + 1) == ports_found) {
            printf("\n");
        }
    }
    // openports.close();

    // /* Save to known ports */
    // std::ofstream list_of_ports_write;
    // list_of_ports_write.open("known_ports.txt", std::ios_base::app);

    // for (int port = 0; port < ports_found; port++) {
    //     list_of_ports_write << open_ports[port] << std::endl;
    // }
    
    // list_of_ports_write.close();
    return 1;

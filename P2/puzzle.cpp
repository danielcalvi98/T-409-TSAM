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


#include <netinet/udp.h>	//Provides declarations for udp header
#include <netinet/ip.h> 	//Provides declarations for ip header
#include <sys/socket.h>     //for socket
#include <netinet/in.h>
#include <arpa/inet.h>      // inet_addr

#include <string.h>     
#include <string>
#include <stdio.h>          //for printf
#include <iostream>
#include <fstream>

#include "scanner_utils.h"  //our utilty module

void checksum_part(char* buffer, int buffersize, sockaddr_in destaddr, int port) {
    int raw_sock;
    timeval timeout;


    /* Create raw socket */
    if ((raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0) {
        perror("Unable to open raw socket, did you forget to run with sudo?");
        return;
    }

    /* Set timeout */
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;

    /* Set options RAW */
    setsockopt(raw_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));

     int opt = 1;
    if (setsockopt(raw_sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0) {
        perror("Setting IP_HDRINCL failed");
        return;
    }

    std::string message = std::string(buffer);
    size_t pos = message.find("0x");
    
    char checksum[6 + 1]; // Guessing it needs +1 for '\0' 

    if (pos == std::string::npos) {
        return;
    }
    for (int i = 0; i < 6; i++) {
        checksum[i] = buffer[i + (int) pos];
    }

    printf("Sending message with checksum...\n");

    bzero(buffer, sizeof(buffer)); // Clear buffer

    /* IP header */
    iphdr *ipheader = (iphdr *) buffer;
    
    /* UDP header */
    udphdr *udpheader = (udphdr *) (sizeof(ip) + buffer);
    
    /* Pseudo header */
    pseudo_header pheader;
    
    /* Data */
    char *data = sizeof(iphdr) + sizeof(udphdr) + buffer;
    strcpy(data, "test");

    /* Source IP */
    char sourceip[32] = "192.168.1.17";

    ipheader -> ihl     = 5;
    ipheader -> version = 4;

    ipheader -> tos     = 0;
    ipheader -> tot_len = sizeof(iphdr) + sizeof(udphdr) + strlen(data);
    ipheader -> id      = htonl(69696);
    ipheader -> ttl     = 255;
    ipheader -> protocol= IPPROTO_UDP;
    ipheader -> check   = 0;
    ipheader -> saddr   = inet_addr(sourceip);
    ipheader -> daddr   = destaddr.sin_addr.s_addr;

    ipheader -> check   = csum((u_short *) buffer, ipheader->tot_len);

    udpheader-> source  = htons(ipheader -> saddr);
    udpheader-> dest    = htons(port);
    udpheader-> len     = htons(8 + strlen(data));
    udpheader-> check   = 0;

    pheader.source      = inet_addr(sourceip);
    pheader.dest        = destaddr.sin_addr.s_addr;
    pheader.zeros       = 0;
    pheader.udp_length  = htons(sizeof(udphdr) + strlen(data));

    int psize = sizeof(pseudo_header) + sizeof(udphdr) + strlen(data);
    char *pseudogram[psize];

    memcpy(pseudogram, (char *) &pheader, sizeof(pseudo_header));
    memcpy(pseudogram + sizeof(pseudo_header), udpheader, sizeof(udphdr) + strlen(data));

    //udpheader-> check   = csum((u_short *) pseudogram, psize);

    u_int16_t old_check = std::stoi(checksum, 0, 16);

    u_int16_t new_check = (old_check>>8) + (old_check<<8);

    udpheader-> check   = new_check; // What we got from the message

    printf("Sending with checksum: %x\n", udpheader -> check);
    /* Send message to buffer */
    if (sendto(raw_sock, buffer, ipheader->tot_len, 0x0, (struct sockaddr *) &destaddr, sizeof(destaddr)) < 0) {
        perror("Failed to send package");
    }

    /* Get response from server */
    bzero(buffer, sizeof(buffer)); // Clear buffer
    int length = recvfrom(raw_sock, buffer, sizeof(buffer), 0x0, NULL, NULL);
    if (length > 0) {
        printf("Got: %s \n", buffer);
    } else {
        perror("Failed to recv package");
    }
}


int main(int argc, char* argv[]) {
    if(argc != 1) {
        printf("Usage: ./puzzle \n");
        exit(0);
    }
    /* Define variables */
    char*   address = (char*) "130.208.243.61";
    int     low =  4000;
    int     high = 4100;

    int     udp_sock;
    char    buffer[1024];
    std::string message;

    struct sockaddr_in  destaddr;
    struct timeval      timeout;

    /* Create upd socket */
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Unable to open socket");
        return -1;
    }

    /* Set timeout */
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;

    /* Set options UDP */
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));

   

    /* Set destination address */
    destaddr.sin_family = AF_INET;
    inet_aton(address, &destaddr.sin_addr);

    for (int port = low; port <= high; port++) {

        int length = send_to_server(buffer, sizeof(buffer), (char*) "$group_42$", udp_sock, destaddr, port);
        if (length) {
            printf("%d: Message : %s\n", port, buffer);
            checksum_part(buffer, sizeof(buffer), destaddr, port);
        }
    }
}

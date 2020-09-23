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


Secret message? St5ctypeIcE

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

    /* Check if message contains 0x AKA the checksum */
    std::string message = std::string(buffer);
    size_t pos = message.find("0x");
    
    char checksum[6 + 1]; // Guessing it needs +1 for '\0'

    if (pos == std::string::npos) {
        return;
    }
    
    strcpy(buffer, "test");
    for (int i = 0; i < 6; i++) {
        checksum[i] = buffer[i + (int) pos];
    }

    int raw_sock;
    timeval timeout;

    // struct sockaddr_in me;
    // me.sin_addr.s_addr = htons(INADDR_ANY);
    // me.sin_family      = AF_INET;
    // me.sin_port        = htons(5000);

    /* Create raw socket */
    if ((raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
        perror("Unable to open raw socket, did you forget to run with sudo?");
        return;
    }
    strcpy(buffer, "test");
    // listen = socket(AF_INET, SOCK_DGRAM, 0);
    // bind(listen, (sockaddr *) me, sizeof(me));

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

    printf("Sending message with checksum...\n");


    /* IP header */
    iphdr *ipheader = (iphdr *) buffer;
    
    /* UDP header */
    udphdr *udpheader = (udphdr *) (sizeof(ip) + buffer);
    
    /* Pseudo header */
    pseudo_header pheader;
    
    /* Data */
    char *data = sizeof(iphdr) + sizeof(udphdr) + buffer;
    strcpy(data, buffer);

    /* Source IP */
    in_addr_t sourceip = inet_addr("192.168.1.17");

    ipheader -> ihl     = 5;
    ipheader -> version = 4;

    ipheader -> tos     = 0;
    ipheader -> tot_len = sizeof(iphdr) + sizeof(udphdr) + strlen(data);
    ipheader -> id      = htonl(500);
    ipheader -> frag_off= htons(0xC000);
    ipheader -> ttl     = 255;
    ipheader -> protocol= IPPROTO_UDP;
    ipheader -> check   = 0; 
    ipheader -> saddr   = sourceip;
    ipheader -> daddr   = destaddr.sin_addr.s_addr;

    ipheader -> check   = csum((u_short *) buffer, ipheader->tot_len); // doesnt make a diffrence

    udpheader-> source  = htons(38509);
    udpheader-> dest    = htons(port);
    udpheader-> len     = htons(sizeof(udphdr) + strlen(data));
    udpheader-> check   = 0;

    pheader.source      = sourceip;
    pheader.dest        = destaddr.sin_addr.s_addr;
    pheader.zeros       = 0; 
    pheader.udp_length  = htons(sizeof(udphdr) + strlen(data));

    int psize = sizeof(pseudo_header) + sizeof(udphdr) + strlen(data);
    char *pseudogram[psize];
    memcpy(pseudogram, (char *) &pheader, sizeof(pseudo_header));
    memcpy(pseudogram + sizeof(pseudo_header), udpheader, sizeof(udphdr) + strlen(data));

    udpheader-> check   = csum((u_short *) pseudogram, psize);
    printf("Calculated checksum: %x\n", udpheader -> check);

    u_int16_t desired_checksum = std::stoi(checksum, 0, 16);
    printf("Required checksum  : %x\n", desired_checksum);
    
    // u_int16_t add = ((desired_checksum - udpheader -> check) + 0xFFFF) % 0xFFFF;
    // char add2 = add &  0x00FF;
    // char add1 = add >> 8;
    // printf("What we need add   : %x\n", add);
    // printf("What we need add   : %c %c\n", add1, add2);

    // strcpy(buffer, "test" + add1 + add2);


    // data = sizeof(iphdr) + sizeof(udphdr) + buffer;
    // strcpy(data, buffer);

    // psize = sizeof(pseudo_header) + sizeof(udphdr) + strlen(data);
    // pseudogram[psize];
    // memcpy(pseudogram, (char *) &pheader, sizeof(pseudo_header));
    // memcpy(pseudogram + sizeof(pseudo_header), udpheader, sizeof(udphdr) + strlen(data));
 
    // udpheader-> check = csum((u_short *) pseudogram, psize);
    // printf("New calculated checksum: %x\n", udpheader -> check);

    //printf("Sending with checksum: %x\n", udpheader -> check);
    /* Send message to buffer */
    if (sendto(raw_sock, buffer, buffersize, 0x0, (struct sockaddr *) &destaddr, sizeof(destaddr)) < 0) {
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

    /* Timeout */
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;

    /* Set options UDP */
    /* Set timeout */
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));

    // /* Dont fragment */
    // int val = IP_PMTUDISC_DO;
    // printf("%x", val);
    // setsockopt(udp_sock, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));

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

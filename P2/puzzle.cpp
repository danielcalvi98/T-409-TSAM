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
#include <sys/ioctl.h>      // VPN
#include <net/if.h>         // VPN

#include <netinet/in.h>
#include <arpa/inet.h>      // inet_addr

#include <string.h>         // memset, strcpy
#include <string>           // std::string
#include <stdio.h>          //for printf



#include "scanner_utils.h"  //our utilty module

int udp_socket(int timeout_ms = 100) {
    int udp_sock;
    struct timeval timeout;
    struct ifreq ifr;

    /* Create upd socket */
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Unable to open socket");
        return -1;
    }

    /* Timeout */
    timeout.tv_sec = 0;
    timeout.tv_usec = timeout_ms * 1000;

    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));

    /* Set VPN */
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "tun0");
    setsockopt(udp_sock, SOL_SOCKET, SO_BINDTODEVICE, (void *) &ifr, sizeof(ifr));

    return udp_sock;
}

char *create_sudo_header(char *data, udphdr *udph, iphdr *iph) {
    struct pseudo_header psh;
    psh.source = iph->saddr;
	psh.dest = iph->daddr;
	psh.zeros = 0;
	psh.protocol = IPPROTO_UDP;
	psh.udp_length = udph->uh_ulen;

    int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
	char *pseudogram = (char *) malloc(psize);
	
    memcpy(pseudogram, (char*) &psh , sizeof (struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));
    return pseudogram;
}

void checksum_part(sockaddr_in destaddr, int port, int checksum) {
	int one = 1;
	const int *val = &one;
    
    int listen = udp_socket();

    bind(listen, (sockaddr *) &destaddr, sizeof(destaddr));

    int s;
    if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
        perror("Error creating socket");
        return;
    }

	if (setsockopt (s, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0) {
		perror("Error setting IP_HDRINCL");
		return;
	}

    struct ifreq ifr;
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "tun0");

    setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr));
    setsockopt(listen, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr));

    //Datagram to represent the packet
    char datagram[4096], *data;
    
    struct iphdr *iph = (struct iphdr *) datagram;

    //UDP header
    struct udphdr *udph = (struct udphdr *) (datagram + sizeof (struct ip));

    for (unsigned short int i = 0; i < 65536; i++) {
    
        //zero out the packet buffer
        memset (datagram, 0, 4096);
        char b = i & 0xff;
        char a = i >> 8;
        char ab[2] = {a,b};
        data = datagram + sizeof(struct iphdr) + sizeof(struct udphdr);
        strcpy(data , (char *) ab);

        iph->ihl = 5;
        iph->version = 4;
        iph->tos = 0;
        iph->tot_len = sizeof (struct iphdr) + sizeof (struct udphdr) + strlen(data);
        iph->id = htonl (123);	//Id of this packet
        iph->frag_off = htons(0xC000);
        iph->ttl = 255;
        iph->protocol = IPPROTO_UDP;
        iph->check = 0;		//Set to 0 before calculating checksum
        iph->saddr = inet_addr("192.168.206.30");	//Spoof the source ip address
        iph->daddr = destaddr.sin_addr.s_addr;

        iph->check = csum((u_short *) datagram, iph->tot_len);

        udph->uh_dport = htons(port);
        udph->uh_sport = htons(6969); // nice
        udph->uh_sum = 0;
        
        udph->uh_ulen = htons(sizeof(struct udphdr) + strlen(data));

        // //Now the UDP checksum
        char *pseudogram = create_sudo_header(data, udph, iph);
        int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);

        udph->uh_sum = csum((u_short*) pseudogram , psize); // correct checksum

        if (ntohs(checksum) == udph->uh_sum) {
            break;
        }
    }    

    if (sendto (s, datagram, iph->tot_len ,	0, (struct sockaddr *) &destaddr, sizeof (destaddr)) < 0) {
        perror("sendto failed");
    } else {
        printf ("Packet Send. Length : %d \n" , iph->tot_len);
    } 
    // sendto(listen, "a", 1, 0, (struct sockaddr *) &destaddr, sizeof (destaddr));

    socklen_t size = sizeof(destaddr);
    char from[255];
    memset(from, 0, sizeof(from));

    if (recvfrom(listen, from, sizeof(from), 0x0, (struct sockaddr *) &destaddr, &size) > 0) {
        printf("Message: %s\n", from);
        return;
    } else {
        perror("No message T-T");
        return;
    }
}


int main(int argc, char* argv[]) {
    if(argc != 1) {
        printf("Usage: ./puzzle \n");
        exit(0);
    }
    /* Define variables */
    char* address = (char*) "130.208.243.61";
    int low =  4000;
    int high = 4100;

    int udp_sock;
    char datagram[1024];
    std::string message;

    struct sockaddr_in destaddr, srcaddr;

    /* Create upd socket */
    udp_sock = udp_socket(100);
    
    /* Set source address */
    srcaddr.sin_family = AF_INET;
    srcaddr.sin_addr.s_addr = INADDR_ANY; // Get my ip
    srcaddr.sin_port = htons(6969);

    bind(udp_sock, (sockaddr *) &srcaddr, sizeof(srcaddr));

    /* Set destination address */
    destaddr.sin_family = AF_INET;
    destaddr.sin_addr.s_addr = inet_addr(address);
    destaddr.sin_port = htons(4010);


    int ports[high - low + 1];

    int nr_of_ports = scan_range(udp_sock, low, high, (char *) "knock", destaddr, datagram, ports);

    for (int p = 0; p <= nr_of_ports; p++) {
        int port = ports[p];
        bzero(datagram, sizeof(datagram));
        send_to_server(datagram, sizeof(datagram), (char *) "$group_42$", udp_sock, destaddr, port);
        printf("%s\n", datagram);
      
        /* Check if message contains 0x AKA the checksum */
        std::string message = std::string(datagram);
        size_t pos = message.find("0x");
        if (pos == std::string::npos) {
            continue;
        }       
        char che[7];
        for (int i = 0; i < 6; i++) {
            che[i] = datagram[i + (int) pos];
        }
        int checksum;
        checksum = std::stoi(che, 0, 16);

        checksum_part(destaddr, port, checksum);  
        printf("%s\n", datagram);
        checksum_part(destaddr, port, checksum);  
        checksum_part(destaddr, port, checksum);  
    }
}

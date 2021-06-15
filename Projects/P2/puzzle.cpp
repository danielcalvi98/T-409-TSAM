/* 
========================================================================
                                 PUZZLE
========================================================================
                                 Authors:
                           Kristofer Robertsson
                            Ymir Thorleifsson
========================================================================
*/

#include <netinet/udp.h>	// Udp header
#include <netinet/ip.h> 	// Ip header
#include <sys/socket.h>     // Sockets
#include <sys/ioctl.h>      // VPN
#include <net/if.h>         // VPN
#include <netinet/in.h>     // Address'
#include <arpa/inet.h>      // Inet_addr
#include <string.h>         // Memset, strcpy
#include <string>           // Std::string
#include <stdio.h>          // Printf

#include "scanner_utils.h"  // Our utilty module

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
    }
}


int main(int argc, char* argv[]) {
    if(argc != 1) {
        printf("Usage: ./puzzle \n");
        exit(0);
    }
    char*   address = (char*) "130.208.243.61";
    int     low =  4000;
    int     high = 4100;
    int     udp_sock;
    char    datagram[1024];
    std::string         message;
    struct sockaddr_in  destaddr, srcaddr;

    /* Create upd socket */
    udp_sock = udp_socket(10);
    
    /* Set source address */
    srcaddr.sin_family      = AF_INET;
    srcaddr.sin_addr.s_addr = INADDR_ANY; // Get my ip
    srcaddr.sin_port        = htons(6969);
    bind(udp_sock, (sockaddr *) &srcaddr, sizeof(srcaddr));
    
    /* Set destination address */
    destaddr.sin_family      = AF_INET;
    destaddr.sin_addr.s_addr = inet_addr(address);
    destaddr.sin_port        = htons(4010);

    std::string secret_ports;
    std::string secret_message;
    int oracle_port;
    int ports[high - low + 1];

    printf("Sailing to the ports...\n\n");
    int nr_of_ports = scan_range(udp_sock, low, high, (char *) "knock", destaddr, datagram, ports);
    printf("🚢.AIRHORN.action.sounds.play() hmm... %d mystery ports...\n", nr_of_ports);

    /*
    /////////////////\\\\\\\\\\\\\\\\\\\
    |||           PHASE 2            |||
    \\\\\\\\\\\\\\\\\///////////////////
    */
    printf("Starting to solve this mystery...\n\n");
    /* Go through the puzzle ports */
    for (int p = 0; p < nr_of_ports; p++) {
        int port = ports[p];
        printf("Solving port %d ", port);
        bzero(datagram, sizeof(datagram));
        send_to_server(datagram, sizeof(datagram), (char *) "$group_42$", udp_sock, destaddr, port);
        // printf("%s\n", datagram);
      
        /* Parse */
        message = std::string(datagram);
        size_t is_the_checksum = message.find("0x"); // Check if message contains 0x AKA the checksum

        /* Modify checksum */
        if (is_the_checksum != std::string::npos) {
            printf("and it's a checksum port.\n");
            char che[7];
            for (int i = 0; i < 6; i++) {
                che[i] = datagram[i + (int) is_the_checksum];
            }
            int checksum;
            checksum = std::stoi(che, 0, 16);

            checksum_part(destaddr, port, checksum);  
            
            bzero(datagram, sizeof(datagram));
            if (recvfrom(udp_sock, datagram, sizeof(datagram), 0x0, NULL, NULL) < 1) {
                perror("No message");
            } 
            // printf("%s\n", datagram);
            message = std::string(datagram);
        }

        /* Extract secret port 1 */
        size_t secret_port_1 = message.find("is 4"); // Asumes 'is 4' is in the message
        if (secret_port_1 != std::string::npos) {
            printf("and it's a secret port, I'll keep this in mind.\n");
            char sp1[5];
            for (int i = 0; i < 4; i++) {
                sp1[i] = datagram[i + (int) secret_port_1 + 3];
            }
            secret_ports += sp1;
            secret_ports += ",";
        }

        /* Extract secret port 2 */
        size_t secret_port_2 = message.find("(dont"); // Asumes '(dont' is in the message
        if (secret_port_2 != std::string::npos) {
            printf("and it's another secret port, I didn't fragment...\n");
            char sp2[5];
            for (int i = 0; i < 4; i++) {
                sp2[i] = datagram[i + (int) secret_port_2 - 4];
            }
            secret_ports += sp2;
        }

        /* Extract secret phrase */
        size_t s_message = message.find("secret phrase:"); // Asumes 'secret phrase: is in the message
        if (s_message != std::string::npos) {
            printf("I set the correct checksum and got a secret phrase...\n");
            for (int i = 0; i < (int) sizeof(datagram); i++) {
                if (message[i + s_message + 17] == '"') {
                    break;
                }
                secret_message += message[i + (int) s_message + 17];
            }
        }

        /* Find oracle port */
        size_t oracle_found = message.find("oracle");
        if (oracle_found != std::string::npos) {
            printf("and it's a oracle port.\n");
            oracle_port = port;
        }

        
    }
    /*
    /////////////////\\\\\\\\\\\\\\\\\\\
    |||           PHASE 3            |||
    \\\\\\\\\\\\\\\\\///////////////////
    */
    printf("\nKnocking on ports...\n\n");

    bzero(datagram, sizeof(datagram));

    /* Send comma seperated list to the oracle */
    send_to_server(datagram, sizeof(datagram), (char *) secret_ports.c_str(), udp_sock, destaddr, oracle_port);
    int ports2[128];
    int nr_of_ports2 = 0;
    char a_port[5];
    int k = 0;
    /* Parse message from oracle */
    for (int i = 0; i < (int) sizeof(datagram); i++) {
        if (datagram[i] == '\0') {
            a_port[k] = *"\0";
            ports2[nr_of_ports2] = atoi(a_port);
            nr_of_ports2++;
            break;
        }
        if (datagram[i] == ',') {
            a_port[k] = *"\0";
            ports2[nr_of_ports2] = atoi(a_port);
            nr_of_ports2++;
            k = 0;
        } else {
            // printf("%c",datagram[i]);
            a_port[k] = datagram[i];
            k++;
        }
    }

    /* You have knocked. You may enter */
    for (int p = 0; p < nr_of_ports2; p++) {
        int port = ports2[p];
        printf("%d Knock! Knock!: ", port);
        bzero(datagram, sizeof(datagram));
        send_to_server(datagram, sizeof(datagram), (char *) secret_message.c_str(), udp_sock, destaddr, port);
        printf("%s\n", datagram);
    }
}

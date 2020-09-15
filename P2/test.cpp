
// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
#define PORT     4097 

  
// Driver code 
int main() { 
    int size_of_message = 1024;
    int sockfd; 
    char buffer[size_of_message]; 
    char *hello = "Hello from client"; 
    struct sockaddr_in     servaddr; 
    
    for (int i = 4042; i <= 4100; i++) {
        // Creating socket file descriptor 
        if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
            perror("socket creation failed"); 
            exit(EXIT_FAILURE); 
        } 
        memset(&servaddr, 0, sizeof(servaddr)); 
        
        // Filling server information 
        servaddr.sin_family = AF_INET; 
        servaddr.sin_addr.s_addr = inet_addr("130.208.243.61");     
        servaddr.sin_port = htons(i); 

        int n; 
        sendto(sockfd, (const char *)hello, strlen(hello), 
            MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
                sizeof(servaddr)); 
        printf("Hello message sent.\n"); 
            
        n = recvfrom(sockfd, (char *)buffer, size_of_message,  
                    MSG_WAITALL, NULL, NULL); 
        if (n == 0) {
            printf("CLOSED");
        } else {
            buffer[n] = '\0'; 
            printf("Server : %s\n", buffer); 
        }
        close(sockfd); 
    }
    return 0; 
} 

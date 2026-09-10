/**
 * This is all from the network programming book 
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>


int main(int c, char ** args){

    /* use of the addrinfo */
    struct addrinfo *result, *rp;

    /*NULL in case of its not a node */
    const struct addrinfo req = {
        .ai_flags =AI_PASSIVE,
        .ai_family = AF_UNSPEC,
    };

    if (getaddrinfo(NULL, "8080", &req, &result) != 0) {
        printf("Error code: %d", errno);
        return -1;
    }

    rp = result; 
    while(rp != NULL) {
        struct sockaddr *addr = rp->ai_addr;

        // cp means contant pointer
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in*) addr; 
            char buffer[INET_ADDRSTRLEN];

            if (inet_ntop(rp->ai_family, &ipv4->sin_addr, buffer, INET_ADDRSTRLEN) == NULL) {
                printf("Something went wrong: %d", errno);
                return -1;
            };
            
            printf("%s\n", buffer);
        
        } else if(rp->ai_family == AF_INET6) {

            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*) addr; 
            char buffer[INET6_ADDRSTRLEN];

            if (inet_ntop(rp->ai_family, &ipv6->sin6_addr, buffer, INET6_ADDRSTRLEN) == NULL) {
                printf("Something went wrong: %d", errno);
                return -1;
            };

            printf("%s\n", buffer);

        } else {
            printf("Uimplemented family: %d\n", rp->ai_family);
            return -1;
        }
        rp = rp->ai_next;
    } 

    return 0;
}

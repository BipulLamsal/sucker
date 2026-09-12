#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

static volatile int run = 1;
static void sig_int_handler(int signum) {
    (void) signum; 
    run = 0;
}

int main(int args_c, char ** args_v) {
    struct sigaction sa = {0};
    sa.sa_handler = sig_int_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; 

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "signal: %s\n",strerror(errno));
        return -1;
    }

    int fd = -1;
    struct addrinfo *result, *rp; /*result is the main needs cleanup*/
    struct addrinfo hints = {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM, /*TCP connection*/
    }; 

    int s = getaddrinfo(NULL, "23", &hints, &result);
    
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return -1;
    }

    void* raw_addr;

    for (rp = result; rp!= NULL; rp=rp->ai_next){
        char dst[INET6_ADDRSTRLEN]; /*Max size*/
        
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) rp->ai_addr;
            raw_addr = &(ipv4->sin_addr);
        } else {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) rp->ai_addr;
            raw_addr = &(ipv6->sin6_addr);
        }
        
        if (inet_ntop(rp->ai_family, raw_addr,dst,  sizeof(dst)) == NULL){
            freeaddrinfo(result);
            fprintf(stderr, "inet_ntop: %s\n", strerror(errno));
            return -1;
        }

        printf("checking inet_ntop: %s\n", dst);
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (fd == -1) {
            fprintf(stderr, "socket: %s\n", strerror(errno));
            continue;
        }

        int opt = 1;
        if (setsockopt(fd,SOL_SOCKET, SO_REUSEADDR ,&opt, sizeof(opt)) < 0){
            fprintf(stderr, "socketopt: %s\n", strerror(errno));
        } 

        /*On success, zero is returned.  On error, -1 is returned, and errno  is  set  to
            indicate the error*/
        if (bind(fd, rp->ai_addr,
              rp->ai_addrlen) == 0) {

            printf("listening: %s:%d\n", dst,23);
            break;
        }

        fprintf(stderr, "bind: %s\n", strerror(errno));
        close(fd);
    }

    /*If its the last element and we still not broke the loop*/
    if  (rp == NULL) {
        freeaddrinfo(result);
        fprintf(stderr, "bind: %s\n", "no address to bind");
        return -1; 
    }

    freeaddrinfo(result);
    int ret = listen(fd,SOMAXCONN);
    if (ret < 0) {
        fprintf(stderr, "listen: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    char buffer[1024];

    while (run) {
        /* accept the connection from the backlog*/
        int client_fd = accept(fd,NULL, NULL);
        /*
         If  the  listen queue is empty of connection requests and O_NONBLOCK is not set
         on the file descriptor for the socket, accept() shall block until a  connection
         is  present.  If  the listen() queue is empty of connection requests and O_NON‐
         BLOCK is set on the file descriptor for the socket, accept() shall fail and set
         errno to [EAGAIN] or [EWOULDBLOCK].
        */

        if (client_fd < 0 ){
            if (errno == EINTR) continue; /* check for interrupt is via continue on while run*/
            fprintf(stderr, "accept: %s\n", strerror(errno));
            continue;
        }

        while (run) {
            memset(buffer, 0, sizeof(buffer));
            
            /*if we have the client fd we can start reading the inputs*/
            ssize_t bytes = recv(client_fd, buffer, 1024, 0);
            if (bytes > 0 ) {
                /* good thing */
                printf("recv: %s", buffer);
            } else if (bytes == 0) {
                /* connection close */
                break;

            } else {
                /* error */ 
                /*errno == EAGAIN || errno == EWOULDBLOCK are somethign to consider*/
                if (errno == EINTR && run) {
                    continue;
                }
                fprintf(stderr, "recv: %s\n", strerror(errno));
                break;
            }
        }
        close(client_fd);
    }


    if (fd >= 0) {
        close(fd);
    }

    return 0;
}

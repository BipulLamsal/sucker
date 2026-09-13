#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sucker.h>


/* returns fd for listening the incoming request */
int create_sucker_listener() {
    int fd = -1; 
    void* raw_addr;
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
    return fd;
} 

/*
 * Make the socket non blocking
 * Returns -1 if it cannot, we switch to blocking
 * Success - 0
**/
int create_sucker_non_block(int fd) {
    if (fcntl(fd,F_SETFL, O_NONBLOCK) < 0) {
        fprintf(stderr, "socketopt: %s\n", strerror(errno));
        return 0;
    }
    return -1;
}
    
void* handle_connection(void *ptr) {
    struct connection_t* arg = (struct connection_t*) ptr;
    char buffer[1024];
    /*because telnet client 1st message is protocol related*/
    ssize_t bytes = recv(arg->client_fd, buffer, 1024, 0);
    (void) bytes;

    while (*(arg->is_running)) {
        /*if we have the client fd we can start reading the inputs*/
        ssize_t bytes = recv(arg->client_fd, buffer, 1024, 0);

        for (int i =0; i < bytes; i++) {
            printf("%c", buffer[i]);
        }

        if (bytes > 0 ) {
            /* good thing */
        } else if (bytes == 0) {
            /* connection close */
            break;

        } else {
            /* error */ 
            /*errno == EAGAIN || errno == EWOULDBLOCK are somethign to consider*/
            if (errno == EINTR && *(arg->is_running)) {
                continue;
            }
            fprintf(stderr, "recv: %s\n", strerror(errno));
            break;
        }
    }

    close(arg->client_fd);
    return NULL;
}

/* Either we get the connection and we add it to our fds, and update the fd_counteor 
 * or we get pollin from the fds itself that is it is sending somthng! we need to read it
 * thats the whole freeaking point of this.
 * */
void check_that_sucker(struct sucker_poll_t* poll_info){
    
    struct pollfd **fds = poll_info->fds;
    int listener_fd = poll_info->listener_fd; 

    for (int i = 0; i<poll_info->fdcount; i++) {
        if ((*fds)[i].revents & (POLLIN | POLL_HUP)) {

            // check if the thing is new client or normal client trying to send some message
            if ((*fds)[i].fd == listener_fd) {
                // accept a new sucker
            } else {
                // read that sucker

            }
        }
    }
}

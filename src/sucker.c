#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sucker.h>

/*
 * Convert socket to IP address string.
 * addr: struct sockaddr_in or struct sockaddr_in6
 */
const char *stringify_that_sucker(void *addr, char *buf, size_t size)
{
    struct sockaddr_storage *sas = addr;
    struct sockaddr_in *sa4;
    struct sockaddr_in6 *sa6;
    void *src;

    switch (sas->ss_family) {
        case AF_INET:
            sa4 = addr;
            src = &(sa4->sin_addr);
            break;
        case AF_INET6:
            sa6 = addr;
            src = &(sa6->sin6_addr);
            break;
        default:
            return NULL;
    }

    return inet_ntop(sas->ss_family, src, buf, size);
}

void get_that_sucker(char*host, char*serv, void* remoteaddr, socklen_t *addrlen){
    if (getnameinfo((struct sockaddr *)&remoteaddr, *addrlen,
                        host, sizeof host, serv, sizeof serv,
                        NI_NUMERICHOST | NI_NUMERICSERV) < 0) {
        fprintf(stderr, "getnameinfo: %s\n", strerror(errno));
    } else {
        printf("sucker connected: %s:%s\n", host, serv);
    }
}

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

    int s = getaddrinfo(NULL, "1234", &hints, &result);
    
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return -1;
    }

    for (rp = result; rp!= NULL; rp=rp->ai_next){
        char dst[INET6_ADDRSTRLEN]; /*Max size*/ 
        
        if (stringify_that_sucker(rp->ai_addr,dst,INET6_ADDRSTRLEN) == NULL){
            fprintf(stderr, "inet_ntop: %s\n", strerror(errno));
            continue;
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
        fprintf(stderr, "fnctl: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
    
int handle_connection(void *ptr, struct sucker_poll_t* poll_info) {
    struct pollfd **fds = poll_info->fds;
    int listener_fd = poll_info->listener_fd;
    int fdcount = poll_info->fdcount;

    struct connection_t* arg = (struct connection_t*) ptr;
    char buffer[1024];
    /*if we have the client fd we can start reading the inputs*/
    ssize_t bytes = recv(arg->client_fd, buffer, 1024, 0);

    if (bytes > 0 ) {
        /* good thing */
        for (int i=0; i<fdcount; i++){
           
            /*skip listener and itself*/
            if ((*fds)[i].fd == listener_fd ||  (*fds)[i].fd == arg->client_fd){
               continue; 
            }
               
            int total = 0; // bytes we have sent
            int left = bytes; 
            while(total < bytes ) {
                    ssize_t sent = send((*fds)[i].fd, buffer+total, left,0);
                    if (sent == -1) {
                        break;
                    }
                    total += sent;
                    left-= sent;
                }
            }
        }
     else if (bytes == 0) {
        /* connection close */
        return -1; 
    } else {
        /* error */ 
        /*errno == EAGAIN || errno == EWOULDBLOCK are somethign to consider*/
        fprintf(stderr, "recv: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

/*
 * Closes the client fd  
 * */
void reap_that_sucker(struct sucker_poll_t* poll_info, int index ){
    struct pollfd **fds = poll_info->fds;
    int fdcount = poll_info->fdcount;
    
    struct pollfd *p;
    struct pollfd *last;
    
    p = *fds + index;
    last = *fds + (fdcount-1);
    *p = *last;

    poll_info->fdcount = fdcount-1;
    close(p->fd);
}

void accept_that_sucker(struct sucker_poll_t* poll_info, void* addr, socklen_t* addr_len ) {
    int fd = accept(poll_info->listener_fd,(struct sockaddr*) addr, addr_len);
    struct pollfd **fds = poll_info->fds;
    
    int reaper_size = poll_info->reaper_size;
    int listener_fd = poll_info->listener_fd;
    int fdcount = poll_info->fdcount;

    if (fd < 0) {
        close(listener_fd);
        fprintf(stderr, "accept: %s\n", strerror(errno));
        return;
    }
    
    if (reaper_size <= fdcount){
        // we need to realloc
        *fds = realloc(*fds, 10 * sizeof (struct pollfd));
        memset((*fds)+fdcount, 0, 10);

        if (*fds == NULL){
            close(listener_fd);
            fprintf(stderr, "realloc: %s\n", strerror(errno));
            return; 
        }
    }
    (*fds)[fdcount].fd= fd;
    (*fds)[fdcount].events = POLLIN;

    poll_info->fdcount += 1;
}

/* Either we get the connection and we add it to our fds, and update the fd_counteor 
 * or we get pollin from the fds itself that is it is sending somthng! we need to read it
 * thats the whole freeaking point of this.
 * */
void check_that_sucker(struct sucker_poll_t* poll_info, int is_running){

    struct pollfd **fds = poll_info->fds;
    int listener_fd = poll_info->listener_fd;
    int fdcount = poll_info->fdcount;


    struct sockaddr_storage remoteaddr; // Client address
    socklen_t addrlen = sizeof remoteaddr;
    char connected_client[INET6_ADDRSTRLEN];

    for (int i = 0; i<fdcount; i++) {
        if ((*fds)[i].revents & (POLLIN | POLL_HUP)) {

            // check if the thing is new client or normal client trying to send some message
            if ((*fds)[i].fd == listener_fd) {
                // accept a new sucker
                // this means we need to add to our exitiing fds list
                // we need to make sure our count is not exceeding
                accept_that_sucker(poll_info, &remoteaddr, &addrlen); 
                char host[NI_MAXHOST], serv[NI_MAXSERV];
                get_that_sucker(host, serv, &remoteaddr, &addrlen);
                
            } else {
                // read that sucker
                pthread_t thread_id;
                struct connection_t conn = {
                    .client_fd = (*fds)[i].fd,
                    .is_running = &is_running,
                };
                int ret = handle_connection(&conn, poll_info);
                if (ret <0) {
                    reap_that_sucker(poll_info, i);
                }
            }
        }
    }
}

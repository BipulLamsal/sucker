#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sucker.h>
#include <pthread.h>
#include <fcntl.h>
 #include <poll.h>

static volatile int run = 1;
static void sig_handler(int signum) {
    switch (signum) {
        case SIGINT:
            run = 0;
        default:
            break;
    }
}

int handles_signals() {
    struct sigaction sa = {0};
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);

    sa.sa_flags = SA_NOCLDSTOP ; 

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "signal: %s\n",strerror(errno));
        return -1;
    }

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        fprintf(stderr, "signal: %s\n",strerror(errno));
        return -1;
    }
    return 0;
}

int main(int args_c, char ** args_v) {
    int default_active_connections = 20;

    if (handles_signals() < 0){
        return -1;
    }
    int listener_fd = create_sucker_listener();
    if (listener_fd<0) {
        return -1;
    }
    
    int ret = listen(listener_fd,SOMAXCONN);
    if (ret < 0) {
        fprintf(stderr, "listen: %s\n", strerror(errno));
        close(listener_fd);
        return -1;
    }
    /*
     * Make the socket non blocking.
     * */
    if(create_sucker_non_block(listener_fd) < 0){
        close(listener_fd);
        return -1;
    }

    struct pollfd* fds;
    fds =  calloc(default_active_connections, sizeof(struct pollfd)); 
    for (int i =0; i<default_active_connections; i++) {
        fds[i].fd = -1;
    }
    fds[0].fd = listener_fd;
    fds[0].events = POLLIN;

    struct sucker_poll_t poll_info = {
        .fds = &fds,
        .fdcount = 1,
        .reaper_size = default_active_connections,
        .listener_fd = listener_fd,
    };
    
    while (run) {
        int poll_count = poll(fds,poll_info.fdcount, -1);
        if (poll_count == -1) {
            fprintf(stderr, "listen: %s\n", strerror(errno));
            close(listener_fd);
            free(fds);
            return -1;
        }
        /* handle the sucker seperately */
        check_that_sucker(&poll_info, run);
    }

    for (int i = 0; i<poll_info.fdcount; i++){
        int fd = poll_info.fds[i]->fd;
        if (fd > 0) {
            close(fd);
        }
    }

    free(fds);
    return 0;
}

/*
 *
 *  accept the connection from the backlog
        int client_fd = accept(listener_fd,NULL, NULL);
        
        
         If  the  listen queue is empty of connection requests and O_NONBLOCK is not set
         on the file descriptor for the socket, accept() shall block until a  connection
         is  present.  If  the listen() queue is empty of connection requests and O_NON‐
         BLOCK is set on the file descriptor for the socket, accept() shall fail and set
         errno to [EAGAIN] or [EWOULDBLOCK].
        

        if (client_fd < 0 ){
            fprintf(stderr, "accept: %s\n", strerror(errno));
            continue;
        }

        pthread_t thread_id;
        struct connection_t conn = {
            .client_fd = client_fd,
            .is_running = &run,
        };

        int ret = pthread_create(&thread_id,NULL,handle_connection,&conn);
        if (ret != 0) {
            close(client_fd);
            fprintf(stderr, "pthread: %s\n", strerror(errno));
            continue;
        }

        pthread_detach(thread_id);
 *
 * */

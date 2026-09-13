#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sucker.h>

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

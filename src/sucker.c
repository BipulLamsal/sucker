#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sucker.h>

void handle_connection(int client_fd, volatile int *is_running) {
    char buffer[1024];
    /*because telnet client 1st message is protocol related*/
    ssize_t bytes = recv(client_fd, buffer, 1024, 0);
    (void) bytes;

    while (*is_running) {
        /*if we have the client fd we can start reading the inputs*/
        ssize_t bytes = recv(client_fd, buffer, 1024, 0);

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
            if (errno == EINTR && *is_running) {
                continue;
            }
            fprintf(stderr, "recv: %s\n", strerror(errno));
            break;
        }
    }

    close(client_fd);
}

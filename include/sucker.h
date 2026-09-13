/*all the requirements for sucker to suck that socket*/

/* handles the client socket like a pro */

struct connection_t {
    int client_fd;
    volatile int *is_running;
};

void *handle_connection( void *ptr );

/*all the requirements for sucker to suck that socket*/

/* handles the client socket like a pro */
#define MAX_CONN 1024

struct sucker_poll_t{
    struct pollfd   **fds;
    int             fdcount;
    int             listener_fd;
};

struct connection_t {
    int             client_fd;
    volatile int    *is_running;
};

int create_sucker_listener();
int create_sucker_non_block(int fd);
void check_that_sucker(struct sucker_poll_t* poll_info);
void *handle_connection( void *ptr );

/*all the requirements for sucker to suck that socket*/

/* handles the client socket like a pro */

struct sucker_poll_t{
    struct pollfd   **fds;
    int             reaper_size;
    int             fdcount;
    int             listener_fd;
};

struct connection_t {
    int             client_fd;
    volatile int    *is_running;

};

int create_sucker_listener();
int create_sucker_non_block(int fd);
void check_that_sucker(struct sucker_poll_t* poll_info,int is_running);
int handle_connection( void *ptr, struct sucker_poll_t* poll_info );
void reap_that_sucker(struct sucker_poll_t* poll_info, int index);

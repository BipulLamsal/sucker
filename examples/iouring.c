#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <liburing.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#define BLOCKSIZE 1024 
#define QD 16

/*
 * Run with gcc iouring.c -luring  
 * */

/*
 *
 * iouring has a ring shared between user space and kernel space.
 * so we are avoiding the idea of copying, zerocopy!
 * shared buffers, shared ring buffers and queues = same thing
 *
 * -> submission queues (place io on this queue)
 * -> completetion queue (kernel place result on this queue)
 *
 *
 * you create a submission queue entry, or SQE, describe the I/O
   operation you need to get done and add it to the tail of
   the submission queue (SQ).
 *  
 *
 *  After you retrieve a CQE, minimally, you
              might be interested in checking the res field 
 * Instead, res will contain what the equivalent system call
              would have returned in case of success, and in case of
              error res will contain -errno.
 

 *  
 *
 *  use of user_data field to estimate which request is it? because its async 
 *  the ordering can be differenet 
 * */

/*
 * Typical steps and mental model:
 *  [SQ - ring buffer] [CQ - ring buffer]
 *  ring buffer?
 *  io_uring_setup(), mmap to mapinto userspace
 *  request -> SQE (add to tail of submission ring buffer)
 *  io_uring_enter() -> singalling that i have entered
 *  optinoally it can also wait for number of requests to be 
 *  processed by the kernel before it returns so you know you're 
 *  ready to read off the completion buffer 
 *
 * */

struct chunk {
    char *buf;
    int res;/* bytes read, or -errno */
};

static int write_all(int fd, const char *buf, size_t len)
{
    while (len > 0) {
        ssize_t w = write(fd, buf, len);
        if (w < 0) {
            return -1;
        }
        buf += w;
        len -= w;
    }
    return 0;
}

static int cat_file(struct io_uring *ring, const char *path)
{
    int ret = -1;
    struct chunk chunks[QD] = {0};

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        goto out;
    }

    off_t nblocks = (st.st_size + BLOCKSIZE - 1) / BLOCKSIZE; 

    for (int i = 0; i < QD; i++) {
        chunks[i].buf = malloc(BLOCKSIZE);
        if (!chunks[i].buf) {
            perror("malloc");
            goto out;
        }
    }

    /* for every chunk we put them in the submission queue */
    for (off_t start = 0; start < nblocks; start += QD) {
        int diff =  nblocks - start; // for chunks less then QD 
        int batch = (diff < QD) ? diff : QD;

        for (int j = 0; j < batch; j++) {
            struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
            io_uring_prep_read(sqe, fd, chunks[j].buf, BLOCKSIZE,
                               (start + j) * BLOCKSIZE);
            io_uring_sqe_set_data(sqe, &chunks[j]); 
        }

        // submit to the kernel for one queue QD
        int submitted = io_uring_submit(ring);
        if (submitted < 0) {
            fprintf(stderr, "submit: %s\n", strerror(-submitted));
            goto out;
        }

        // we submitted batch in the flight now we need wait all of them one
        // by one
        for (int j = 0; j < batch; j++) {
            struct io_uring_cqe *cqe;
            int r = io_uring_wait_cqe(ring, &cqe);
            if (r < 0) {
                fprintf(stderr, "wait_cqe: %s\n", strerror(-r));
                goto out;
            }
            /* get user_data */
            struct chunk *c = io_uring_cqe_get_data(cqe);
           /* this is the same file we preivously 
            * set so we set res we got from temp cqe*/ 
            c->res = cqe->res;
            io_uring_cqe_seen(ring, cqe);
        }

        for (int j = 0; j < batch; j++) {
            if (chunks[j].res < 0) {
                fprintf(stderr, "%s: read: %s\n", path,
                        strerror(-chunks[j].res));
                goto out;
            }
            if (write_all(STDOUT_FILENO, chunks[j].buf, chunks[j].res) < 0) {
                perror("write");
                goto out;
            }
        }
    }
    ret = 0;

out:
    for (int i = 0; i < QD; i++)
        free(chunks[i].buf);
    close(fd);
    return ret;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file [file ...]\n", argv[0]);
        return 1;
    }

    struct io_uring ring;
    int r = io_uring_queue_init(QD, &ring, 0);
    if (r < 0) {
        fprintf(stderr, "queue_init: %s\n", strerror(-r));
        return 1;
    }

    int status = 0;
    for (int i = 1; i < argc; i++)
        if (cat_file(&ring, argv[i]) < 0)
            status = 1;

    io_uring_queue_exit(&ring);
    return status;
}

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define MAX_EVENTS 10

/*
 * Level or edge triggered? interface?
 * interenst set and readt sets (ready for IO through kernel) is refernce to interest set itself, populated by the kernel through IO activity.
 *
 * */

/*
 * is this level triggerd?
 *
 * -> read side is registed means we are that 
 * -> writer writes 2kb 
 * -> so epoll_wait is used that returns ready fd (read sides)
 *  -> the pipe reader reads 1kb of the data alr
 *  -> call to epoll_wait is done?
 *  if the flag was: EPOLLET (edge triggered)
 *  the above step would hang desipete the data is present 1 because it sees spike in data change and triggers 
 *  the event but there can be situtation where buffer has some bytes already existing which will not trigger any event 
 *  becuase it only consider changes, or when new data arrieves, thats it.1
 * 
 * EPOLLONESHOT to rearm the threads 
 * (EPOLL_CTL_MOD)
 * if edge triggered is enabled then it wakes up only a threads becuase it takes out of the ready list and other threads 
 * doensont wake for it.
 *
 * */
const char *addr_to_str(const struct sockaddr_storage *ss, char *buf, size_t buflen) {
    char ip[INET6_ADDRSTRLEN];

    if (ss->ss_family == AF_INET) {
        const struct sockaddr_in *sin = (const struct sockaddr_in *)ss;
        inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof(ip));
        snprintf(buf, buflen, "%s:%d", ip, ntohs(sin->sin_port));
    } else if (ss->ss_family == AF_INET6) {
        const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)ss;
        inet_ntop(AF_INET6, &sin6->sin6_addr, ip, sizeof(ip));
        snprintf(buf, buflen, "[%s]:%d", ip, ntohs(sin6->sin6_port));
    } else {
        snprintf(buf, buflen, "unknown family %d", ss->ss_family);
    }
    return buf;
}

int main(void) {
    struct addrinfo hints = {
        .ai_family   = AF_INET,
        .ai_socktype = SOCK_STREAM,
        .ai_flags    = AI_PASSIVE,
    };
    struct addrinfo *result;
 
    int ret = getaddrinfo(NULL, "1997", &hints, &result);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return 1;
    }
 
    int server_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (server_fd == -1) {
        perror("socket");
        freeaddrinfo(result);
        return 1;
    }
 
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
 
    if (bind(server_fd, result->ai_addr, result->ai_addrlen) != 0) {
        perror("bind");
        freeaddrinfo(result);
        close(server_fd);
        return 1;
    }
    freeaddrinfo(result);
 
    if (listen(server_fd, 1024) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }
    printf("server running: 0.0.0.0:1997\n");
 
    struct epoll_event ev, rev, events[MAX_EVENTS];
 
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        close(server_fd);
        return 1;
    }
 
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
 
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: server_fd");
        close(epollfd);
        close(server_fd);
        return 1;
    }
 
    for (;;) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
 
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
 
            if (fd == server_fd) {
                struct sockaddr_storage addr;
                socklen_t sock_len = sizeof(addr);
 
                int client_fd = accept(server_fd, (struct sockaddr *)&addr, &sock_len);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }
 
                fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK);
 
                char s[INET6_ADDRSTRLEN + 8];
                printf("fd %d connected: %s\n", client_fd,
                       addr_to_str(&addr, s, sizeof(s)));
 
                ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;;
                ev.data.fd = client_fd;
 
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("epoll_ctl: client_fd");
                    close(client_fd);
                    continue;
                }
 
            } else {
                char buf[2];
                for (;;) {
                    ssize_t r = recv(fd, buf, sizeof(buf), 0);
                    if (r > 0) {
                        printf("fd %d: got %zd bytes: %.*s\n", fd, r, (int)r, buf);

                        size_t sent = 0;
                        int fd_closed = 0;
                        while (sent < r) {
                            ssize_t s = send(fd, buf, r, MSG_NOSIGNAL);
                            if (s>0) {
                                sent += s;
                            } else if (s == -1 && errno == EAGAIN) {
                                // done sending but the buffer is full so we drop?
                                break;
                            }else {
                                perror("send");
                                close(fd);
                                fd_closed = 1;
                                break;
                            }
                        }

                        if (fd_closed) break;

                    } else if (r == 0) {
                        close(fd);          // also removes it from epoll
                        break;
                    } else {
                        if (errno == EAGAIN)
                            // rearm the fd
                            rev.events  = EPOLLIN | EPOLLET | EPOLLONESHOT; 
                            rev.data.fd = fd;
                            epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &rev);


                            break;          // drained, back to epoll_wait
                        perror("recv");
                        close(fd);
                        break;
                    }
                }


            }
        }
    }
 
    close(epollfd);
    close(server_fd);
    return 0;
}


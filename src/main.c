/**
 * This is all from the network programming book 
 */
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <linux/input.h>

int main(int c, char ** args){
    int fd = open("/dev/input/event4", O_RDONLY);
    
    if(fd <0) {
        printf("%s\n", strerror(errno));
        return -1;
    }


    struct input_event event;

    
    for(;;) {
        if (read(fd,&event, sizeof(event)) < 0){
            printf("%s\n", strerror(errno));
            return -1;
        };

        printf("key %u\n", event.code);
    }

    return 0;
}

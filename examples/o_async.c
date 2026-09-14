/*
 * Just a curosiry driven i lowkey want to see differnece between
 * O_NONBLOCK & O_ASYNC, sounds similary how do they differ?
 * */
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

#define STDIN 0

void handler(int sig) {
    if(sig & SIGIO) {
        printf("you pressed it right? \n");
    }
}

int main(){
    /*Alrighty?  This feature is available only for terminals, pseudoterminals,*/

    /*So I am in my line buffered terminial and waiting for STDIN is possible with O_ASYNC*/
    
    signal(SIGIO, handler);
    fcntl(STDIN, F_SETFL, O_ASYNC ); 
    

    char buffer[100];
    ssize_t received =  read(STDIN, buffer, 100);
    printf("%s", buffer);
    return 0;
}

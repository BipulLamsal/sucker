#include <stdlib.h>
#include <stdio.h>
struct user{
    int id;
    char* name;
};

void mutate_user(struct user** upp){
    
    printf("---> %d\n", ((*upp) + 2 )->id);
    struct user* up;
    struct user temp;
    up = *upp; // array 
    
    temp = *up;
    *up = up[2];
    up[2] = temp;

}

void main() {
    struct user* u;
    u = calloc(10, sizeof(struct user));
    struct user bipul =  {
        .id = 2,
        .name= "Bipul",
    };
    u[2] = bipul;

    mutate_user(&u);
    printf("id: %d\n", u[0].id);
    free(u);
}

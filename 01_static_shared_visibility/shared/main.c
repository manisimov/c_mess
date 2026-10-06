#include <stdio.h>

extern int shared_global_without(int b);
extern int shared_static_without(int b);
extern int shared_global_hidden(int b);
extern int shared_global_default(int b);

int main(void) {
    shared_global_without(1);
    //shared_static_without(2);
    //shared_global_hidden(3);
    shared_global_default(4);
    printf("hi\n");
    return 0;
}

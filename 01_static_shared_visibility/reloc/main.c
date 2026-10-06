#include <stdio.h>

extern int reloc_global_without(int b);
extern int reloc_static_without(int b);
extern int reloc_global_hidden(int b);
extern int reloc_global_default(int b);

int main(void) {
    reloc_global_without(1);
    //reloc_static_without(2);
    reloc_global_hidden(3);
    reloc_global_default(4);
    printf("hi\n");
    return 0;
}

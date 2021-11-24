#include "kernel/types.h"
#include "user.h"

int
main(int argc,char *argv[])
{
    int second;
    if (argc!=2) {
        fprintf(2,"Usage: sleep [second]\n");
        exit(1);
    }
    second = atoi(argv[1]);
    if (sleep(second) < 0 ) {
        fprintf(2,"syscall sleep error");
        exit(1);
    }

//    return 0;
    exit(0);
}
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
//    printf("x=%d y=%d", 3);
    int b = 0xff;
    int a = 0x00646c72;
    a = 0x72;
    int c = 0x0;
    int bytes = sizeof(int);
    while (bytes!=0) {
        c = c | (a & b );
        a = a >> 8;
        bytes-=1;
        if (bytes!=0) {
            c = c << 8;
        }
    }
    printf("c=%x , a=%x",c,a);

  exit(0);
}

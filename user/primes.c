#include "kernel/types.h"
#include "user/user.h"

void
filter(int lpipe[])
{
    int rpipe[2];
    if (pipe(rpipe) <0 ) {
        printf("syscall pipe error");
        exit(1);
    }

    int primes[50];
    int cnt = 0;
    int buf;

    close(lpipe[1]);

    while ((read(lpipe[0], &buf, sizeof(buf))) != 0) {
        primes[cnt++] = buf;
//        printf("4");
    }
    close(lpipe[0]);

    if (cnt == 0) {
        printf("cnt==0");
        return;
    }
    int first = primes[0];
    printf("prime %d\n", first);

    int pid = fork();

    if (pid == 0) {
        //child
        filter(rpipe);

    } else {

        for (int i = 1; i < cnt; i++) {
            if (primes[i] % first != 0) {
                if (write(rpipe[1], &primes[i], sizeof(primes[i]))<0) {
                    printf("syscall write error 1");
                    exit(1);
                }
            }
        }
        close(rpipe[1]);
        wait((int *) 0);
    }
}

int
main()
{
    int lpipe[2];

    if (pipe(lpipe) <0) {
        printf("syscall pipe error");
        exit(1);
    }

    int pid = fork();

    if (pid == 0) {
        //consumer
//        printf("1");
        filter(lpipe);
    } else {
        //producer
//        printf("2");
        for (int i = 2; i < 35; i++) {
            write(lpipe[1],&i,sizeof(i));
//            printf("3");
        }
        close(lpipe[1]);
        wait((int *) 0);
    }
    exit(0);
}
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc,char *argv[])
{
    int first_fd[2];
    int second_fd[2];
    char buf;

    if (pipe(first_fd) <0 || pipe(second_fd)<0) {
        fprintf(2,"syscall pipe error");
        exit(1);
    }

    int pid = fork();

    //child
    if (pid == 0) {
        close(first_fd[1]);
        close(second_fd[0]);
        read(first_fd[0], &buf, 1);
        if (buf == 'o') {
            printf("%d: received ping\n", getpid());
        }
        write(second_fd[1], "o", 1);
        close(first_fd[0]);
        close(second_fd[1]);
    } else {
        //parent
        close(first_fd[0]);
        close(second_fd[1]);
        write(first_fd[1], "o", 1);
        read(second_fd[0], &buf, 1);
        if (buf =='o') {
            printf("%d: received pong\n", getpid());
        }
        close(first_fd[1]);
        close(second_fd[0]);
    }
    exit(0);
}
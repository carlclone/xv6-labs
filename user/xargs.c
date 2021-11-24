#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
    char *nargs[MAXARG];
    int arg_count = 0;
    for (int i = 1; i < argc; i++) {
        nargs[arg_count] = argv[i];
        arg_count++;
    }

    char tmp_char;
    char *buf;
    char tmp_buf[100];
    buf = tmp_buf;
    int pos=0;
    while (read(0, &tmp_char, 1) == 1) {
        if (tmp_char == '\n') {
            buf[pos] = 0;
            nargs[arg_count] = buf;
            char tmp_buf[100];
            buf = tmp_buf;
            arg_count++;
            pos=0;
        } else {
            buf[pos]= tmp_char;
            pos++;
        }
    }

    if (fork() == 0) {
        exec(nargs[0], nargs);
    } else {
        wait(0);
    }
    exit(0);
}
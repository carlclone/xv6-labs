#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "user/common.h"
//all in stack , be careful

void
get_file_path(char *prefix, char *name, char *buf)
{
    memcpy(buf, prefix, strlen(prefix));
    char *p = buf + strlen(prefix);
    *p = '/';
    p++;
    memcpy(p, name, strlen(name));
    p += strlen(name);
    *p = 0;
}

void
find(int fd, char *dir, char *name)
{
    struct dirent de;

    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
            continue;
        }
        struct stat st;
        char path[512];
        get_file_path(dir, de.name, path);

        //
        if (de.inum == 0) {
            continue;
        }

        if (stat(path, &st) < 0) {
            printf("syscall stat error,path:%s\n",path);
            continue;
        }
        if (st.type==T_FILE && match(de.name,name)) {
            printf("%s\n", path);
        } else if (st.type == T_DIR) {
            int subfd;
            if ((subfd = open(path,0))<0) {
                printf("syscall open error,path:%s\n", path);
                continue;
            }
            find(subfd, path, name);
        }
    }

}

int
main(int argc,char *argv[]) {
    if (argc != 3) {
        fprintf(2, "Usage: find [dir]\n");
        exit(1);
    }

    char dir[DIRSIZ + 1];
    char name[DIRSIZ + 1];

    if (strlen(argv[1]) > DIRSIZ || strlen(argv[2]) > DIRSIZ) {
        fprintf(2, "dir or name too long\n");
        exit(1);
    }

    memcpy(dir, argv[1], strlen(argv[1]));
    memcpy(name, argv[2], strlen(argv[2]));

    int fd;
    struct stat st;

    if ((fd = open(dir, 0)) < 0) {
        fprintf(2, "syscall open error");
        exit(1);
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "syscall fstat error");
        exit(1);
    }

    if (st.type != T_DIR) {
        printf("%s is not a dir\n", dir);
    } else {
        find(fd, dir, name);
    }
    exit(0);
}
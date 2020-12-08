#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char* argv[]) {
    int fd = open("tmp.txt", O_RDWR|O_CREAT|O_TRUNC, 0600);
    execlp(argv[1], "ls", ">", "tmp.txt", NULL);
    execlp("tmp.txt", "wc", "-w", NULL);
    remove("tmp.txt");
    return 0;
}
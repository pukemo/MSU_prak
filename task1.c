#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int pipedes[2];

int main (unsigned int argc, char* argv[]) {
    
    pid_t pid1, pid2;
    if (argc < 2) {
        return 1;
    }
    
    pipe(pipedes);
    if (!(pid1 = fork())) {
        dup2(pipedes[1], 1);
        close(pipedes[0]);
        execl("bin/ls", "ls", argv[1], NULL);
        exit(1);
    }
    if (!(pid2 = fork())) {
        dup2(pipedes[0], 0);
        execl("bin/wc", "wc", "-w", NULL);
        exit(1);
    }
    
    close (pipedes[0]);
    close (pipedes[1]);
    
    wait(-1);
    wait(-1);
    
    return 0;
}

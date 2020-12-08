#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char* argv[]) {
    if(argc < 2) {return(0);}
    int fd = open("tmp.txt", O_RDWR|O_CREAT|O_TRUNC, 0600); 
    system("ls -l > tmp.txt"); 
    system("wc -w tmp.txt")
    remove("tmp.txt");
    return 0;
}

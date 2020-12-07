#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

void redirect_stream_to_file(FILE* stream, char* file_name, char* mode, int* fd, fpos_t* pos) {
    fflush(stream);
    fgetpos(stream, pos);
    *fd = dup(fileno(stream));
    freopen(file_name, mode, stream);
}

void restore_stream(FILE* stream, int fd, fpos_t pos) {
    fflush(stream);
    dup2(fd, fileno(stream));
    close(fd);
    clearerr(stdout);
    fsetpos(stdout, &pos);
}


int main(int argc, char **argv)
{
    int fd[2];
    pipe(fd);        /* организовываем канал */
 
    if (fork() == 0) {
        if (fork() == 0) /* иначе print может завершиться до WC и Shell выдаст приглашение до завершения WC */
        {
            /* ПРОЦЕСС- сын */
            /* отождествим стандартный вывод с файловым дескриптором канала, предназначенным для записи */
            dup2(fd[1], 1);
            /* за./крываем файловый дескриптор канала, предназначенный для записи */
            /* закрываем файловый дескриптор канала, предназначенный для чтения */
            close(fd[0]);
            /* запускаем программу print */
            // execlp("cat", "3.txt", NULL);
            system("cat 2.txt");
            close(fd[1]);
            exit(0);
        } else {
            /* ПРОЦЕСС-родитель */
            /*отождествляем стандартный ввод с файловым дескриптором канала, предназначенным для чтения */
            dup2(fd[0], 0);
            /* закрываем файловый дескриптор канала, предназначенный для чтения */
            close(fd[0]);
            /* закрываем файловый дескриптор канала, предназначенный для записи */
            close(fd[1]);
            /* запускаем программу wc */
            // execlp("wc", "wc", NULL);
            system("wc");
            exit(0);
        }
    }
    printf("That's all, folks!");
    return 0;
}
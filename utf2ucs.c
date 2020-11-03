#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

FILE *check_file_read(char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("Can't open file ");
        perror(filename);
        perror("!\n");
        return NULL;
    }
    return f;
}

int check_bom_utf(char* filename) {
    // return -1 if can't open file, else 0
    int res;
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("Can't open output file!\n");
        return -1;
    }
    unsigned char c1, c2, c3;
    if ((fread(&c1, 1, 1, f) != 1) || (fread(&c2, 1, 1, f) != 1) || (fread(&c3, 1, 1, f) != 1)) {
        perror("Incorrect BOM!\n");
    }
    //printf("%x %x %x\n", c1, c2, c3);
    if((c1 != 0xEF) || (c2 != 0xBB) || (c3 != 0xBF)) {
        perror("Incorrect BOM!\n");
    }
    fclose(f);
    return 0;
}
void process_file(FILE *f1,FILE *f2,int bom)
{
    int i,fb=0,sb=0;
    unsigned char c,j;
    unsigned short ch=0,a;
    while ((i=fread(&c,sizeof(char),1,f1))==1){
        if ((j=c&0X80)==0){
            if (fb!=0){
                fprintf(stderr,"incorrect symbol:%c\n",c);
                fprintf(stderr,"Offset (beginning of the file):%ld",ftell(f1));
                fb=0;
                sb=0;
                ch=0;
            }
            else{
                ch=c;
                if (bom==0){
                    a=ch&0x00ff;
                    a=a<<8;
                    ch=ch>>8;
                    ch=ch|a;
                }
                fwrite(&ch,sizeof(unsigned short),1,f2);
                ch=0;
            }
        }
        else if (fb==110&&(j=c&0xc0)==0x80){
            c=c&0x3F;
            a=c;
            ch=ch|a;
            if (bom==0){
                a=ch&0x00ff;
                a=a<<8;
                ch=ch>>8;
                ch=ch|a;
            }
            fwrite(&ch,sizeof(unsigned short),1,f2);  
            ch=0;
            fb=0;
        }
        else if (fb==111&&sb==10&&(j=c&0x80)==0x80){
            c=c&0x3F;
            a=c;
            ch=ch|a;
            if (bom==0){
                a=ch&0x00ff;
                a=a<<8;
                ch=ch>>8;
                ch=ch|a;
            }
            fwrite(&ch,sizeof(unsigned short),1,f2);
            ch=0;
            fb=0;
            sb=0;
        }
        else{
            if ((j=c&0xe0)==0xc0){
                if (fb==110){
                    fprintf(stderr,"incorrect symbol:%c\n",c);
                    fprintf(stderr,"Offset (beginning of the file):%ld",ftell(f1)-1);
                }
                c=c&0x1F;
                ch=c;
                ch=ch<<6;
                fb=110;
            }  
            else if ((j=c&0xe0)==0xe0){
                if (fb==111){
                    fprintf(stderr,"incorrect symbol:%c\n",c);
                    fprintf(stderr,"Offset (beginning of the file):%ld",ftell(f1)-1);
                }
                c=c&0x0F;
                ch=c;
                ch=ch<<12;
                fb=111;
            }  
            else if ((j=c&0xc0)==0x80){
                if (fb==0){
                    fprintf(stderr,"incorrect symbol:%c\n",c);
                    fprintf(stderr,"Offset (beginning of the file):%ld",ftell(f1));
                }
                else{
                    c=c&0x3F;
                    a=c;
                    a=a<<6;
                    ch=ch|a;
                    sb=10;
                }
            }
        }
    }
}

int main(int argc, char** argv) {

    FILE* fin = stdin;
    FILE* fout = stdout;
    int check_bom;

    if (argc > 3) {
        perror("Invalid number of input arguments!\n");
        return 0;
    }
    if (argc == 2) {
        fin = check_file_read(argv[1]);
    } else if (argc == 3) {
        fin = check_file_read(argv[1]);
        check_bom=check_bom_utf(argv[1]);
        fout = fopen(argv[2], "w");
    }

    if (fin == NULL || fout == NULL) {
        return 0;
    }
    if (check_bom == -1) {
        fclose(fin);
        return 0;
    }
    process_file(fin, fout, check_bom);
    fclose (fin);
    fclose (fout);
    
    return 0;
}
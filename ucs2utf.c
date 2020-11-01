#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

FILE* check_file_read(char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("Can't open file ");
        perror(filename);
        perror("!\n");
        return NULL;
    }
    return f;
}

int check_bom_ucs(FILE* f) {
    // return 0 - BE, 1 - LE, <0 else
    unsigned char c1, c2;
    if (fread(&c1, 1, 1, f) != 1 || fread(&c2, 1, 1, f) != 1) {
        perror("Can't read BOM!\n");
        return -1;
    }
    if ((c1 == 0xFF) && (c2 == 0xFE)) {
        return 1;
    } else if ((c1 == 0xFE) && (c2 == 0xFF)) {
        return 0;
    } else {
        // no BOM
        fseek(f, 0, SEEK_SET);
        return 1;
    }
}


void write_bom_utf(FILE* f) {
    fseek(f, 0, SEEK_SET);
    unsigned char c1 = 0xEF, c2 = 0xBB, c3 = 0xBF;
    fwrite(&c1, 1, 1, f);
    fwrite(&c2, 1, 1, f);
    fwrite(&c3, 1, 1, f);
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


void process_file(FILE* fin, FILE* fout, int le) {
    unsigned char c1, c2;
    while (!feof(fin)) {
        int r1 = fread(&c1, 1, 1, fin);
        int r2 = fread(&c2, 1, 1, fin);
        if (r1 == 0 || r2 == 0) {
            perror("incorrect number of bytes in input ucs file!\n");
            return;
        }
        if (le) {
            unsigned char c3 = c1;
            c1 = c2;
            c2 = c3;
        }
        unsigned short x = ((unsigned short) c1 << 8) + c2;
        if (x < 0x80) {
            // one-byte word
            unsigned char b1 = (unsigned char) x;
            fwrite(&b1, 1, 1, fout);
        } else if ((x >= 0x80) && (x < 0x800)) {
            // two-byte word
            unsigned char b1 = (unsigned char)(x >> 6) + 0xC0;
            unsigned char b2 = (unsigned char)(x & 0x3F) + 0x80;
            fwrite(&b1, 1, 1, fout);
            fwrite(&b2, 1, 1, fout);
        } else {
            // three-byte word
            unsigned char b1 = (unsigned char)(x >> 12) + 0x00E0;
            unsigned char b2 = (unsigned char)((x >> 6) & 0x003F) + 0x80;
            unsigned char b3 = (unsigned char)(x & 0x003F) + 0x80;
            fwrite(&b1, 1, 1, fout);
            fwrite(&b2, 1, 1, fout);
            fwrite(&b3, 1, 1, fout);
        }
    }
    fclose(fin);
    fclose(fout);
}


int main(int argc, char** argv) {

    FILE* fin = stdin;
    FILE* fout = stdout;

    if (argc > 3) {
        perror("Invalid number of input arguments!\n");
        return 0;
    }
    if (argc == 2) {
        fin = check_file_read(argv[1]);
    } else if (argc == 3) {
        fin = check_file_read(argv[1]);
        check_bom_utf(argv[2]);
        fout = fopen(argv[2], "w");
    }

    if (fin == NULL || fout == NULL) {
        return 0;
    }
    int bom = check_bom_ucs(fin);
    if (bom == -1) {
        fclose(fin);
        return 0;
    }
    printf("%s encoding is used\n", bom ? "LE" : "BE");
    write_bom_utf(fout);
    process_file(fin, fout, bom);


    return 0;
}
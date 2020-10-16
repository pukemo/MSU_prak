#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define MAX_WORD_LENGTH 100
#define INITIAL_WORD_LENGTH 8


typedef struct Node {
    struct Node* left;
    struct Node* right;
    char* word;
    int counter;
} Node;


void tree_print(Node* tree) {
    if (tree == NULL) {
        return;
    }
    printf("word: %s (%d), left = %p, right = %p;\n", tree->word, tree->counter, tree->left, tree->right);
    tree_print(tree->left);
    tree_print(tree->right);
}

Node* copy_node(Node* node) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->word = node->word;
    new_node->counter = node->counter;
    new_node->left = NULL;
    new_node->right = NULL;
    return new_node;
}

void tree_append_2(Node** tree, Node* node) {
    if (*tree == NULL) {
        *tree = node;
        return;
    }
    if (node->counter <= (*tree)->counter) {
        tree_append_2(&((*tree)->left), node);
    } else {
        tree_append_2(&((*tree)->right), node);
    }
}

void tree_copy_and_rearrange(Node* tree, Node** new_tree) {
    if (tree == NULL) {
        return;
    }
    Node* new_node = copy_node(tree);
    tree_append_2(new_tree, new_node);
    tree_copy_and_rearrange(tree->left, new_tree);
    tree_copy_and_rearrange(tree->right, new_tree);
}

void tree_print_to_file(FILE* fout, Node* tree, int number_of_words) {
    if (tree == NULL) {
        return;
    }
    tree_print_to_file(fout, tree->right, number_of_words);
    fprintf(fout, "%s %d %lf\n", tree->word, tree->counter, (double) tree->counter / number_of_words);
    tree_print_to_file(fout, tree->left, number_of_words);
}


void tree_append(Node** tree, char* new_word) {
    if (*tree == NULL) {
        // create new node for word new_word (counter = 1)
        Node* new_node = (Node*)malloc(sizeof(Node));
        new_node->word = (char*)malloc(strlen(new_word) + 1);
        strcpy(new_node->word, new_word);
        new_node->counter = 1;
        new_node->left = NULL;
        new_node->right = NULL;
        *tree = new_node;
        return;
    }
    int cmp = strcmp(new_word, (*tree)->word);
    if (cmp == 0) {
        (*tree)->counter++;
        return;
    } else if (cmp < 0) {
        tree_append(&((*tree)->left), new_word);
    } else {
        tree_append(&((*tree)->right), new_word);
    }
}


void tree_delete(Node* tree, int delete_word) {
    if (tree == NULL) {
        return;
    }
    tree_delete(tree->left, delete_word);
    tree_delete(tree->right, delete_word);
    if (delete_word) {
        free(tree->word);
    }
    free(tree);
}

void append_to_word(char** word, char symbol, int* counter, int* reserved, int* is_empty) {
    if (*counter == *reserved - 1) {
        *word = (char*) realloc(*word, (*reserved) * 2);
        *reserved *= 2;
    }
    (*word)[*counter] = symbol;
    (*word)[*counter + 1] = '\0';
    (*counter)++;
    *is_empty = 0;
}

void reset_word(char* word, int* counter, int* is_empty) {
    word[0] = '\0';
    *counter = 0;
    *is_empty = 1;
}

int parse_input_by_char(FILE* fin, Node** tree) {
    int total = 0; // total number of words in file
    char* word = (char*) malloc(INITIAL_WORD_LENGTH);
    int counter = 0; // number of symbols in word
    int reserved = INITIAL_WORD_LENGTH;
    word[0] = '\0';
    int is_empty = 1;
    char c;
    while ((c = fgetc(fin)) != EOF) {
        if (c == ' ' || c == '\t' || c == '\n') {
            if (!is_empty) {
                tree_append(tree, word);
                total++;
            }
            reset_word(word, &counter, &is_empty);
        } else if (c == '.' || c == ',' || c == ':' || c == '(' || c == ')' || c == '!' || c == '?' || c == '"' || c == '%') {
            // finish previous word
            if (!is_empty) {
                tree_append(tree, word);
                total++;
            }
            // form new word with this symbol
            reset_word(word, &counter, &is_empty);
            append_to_word(&word, c, &counter, &reserved, &is_empty);
            tree_append(tree, word);
            total++;
            // reset word
            reset_word(word, &counter, &is_empty);
        } else {
            append_to_word(&word, c, &counter, &reserved, &is_empty);
        }
    }
    free(word);
    return total;
}


int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Incorrect input. 4 arguments expected.\n");
        return 0;
    }
    if (strcmp(argv[1], "-i") != 0 || strcmp(argv[3], "-o") != 0) {
        printf("Incorrect input. Input has to be: -i file_in.txt -o file_out.txt\n");
        return 0;
    }
    char* filename_in = argv[2];
    char* filename_out = argv[4];

    FILE* fin = fopen(filename_in, "r");
    if (!fin) {
        printf("INput file opening failed\n");
        return 0;
    }

    Node* tree_alpha = NULL;
    Node* tree_counter = NULL;

    int number_of_words = parse_input_by_char(fin, &tree_alpha);
    fclose(fin);

    tree_copy_and_rearrange(tree_alpha, &tree_counter);

    FILE* fout = fopen(filename_out, "w");
    if (!fout) {
        printf("Output file opening failed\n");
        return 0;
    }

    // tree_print(tree);
    //tree_print_to_file(fout, tree_alpha, number_of_words);
    tree_print_to_file(fout, tree_counter, number_of_words);

    fclose(fout);

    tree_delete(tree_alpha, 1);
    tree_delete(tree_counter, 0);

    return 0;
}
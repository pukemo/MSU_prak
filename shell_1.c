#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_WORD_LENGTH 4
#define DEFAULT_CMD_ARGS 2
#define MAX_PATH_LENGTH 256


typedef struct state {
    int find_new_arg;
    int inside_quotes;
} state;

typedef struct word {
    char* data;
    int reserved;
    int length;
} word;

typedef struct command {
    word** args;
    int reserved;
    int length;
} command;


void initalize_state(state* st) {
    st->find_new_arg = 1;
    st->inside_quotes = 0;
}

void initialize_word(word* w) {
    w->data = (char*) malloc(DEFAULT_WORD_LENGTH * sizeof(char));
    w->data[0] = '\0';
    w->length = 0;
    w->reserved = DEFAULT_WORD_LENGTH;
}

void append_char_to_word(word* w, char c) {
    if (w->length == w->reserved - 1) {
        w->data = (char*) realloc(w->data, w->reserved * 2);
        w->reserved *= 2;
    }
    w->data[w->length] = c;
    w->data[w->length + 1] = '\0';
    w->length++;
}

void delete_word(word* w) {
    free(w->data);
    free(w);
}

void initialize_command(command* cmd) {
    cmd->args = (word**) malloc(DEFAULT_CMD_ARGS * sizeof(word*));
    cmd->length = 0;
    cmd->reserved = DEFAULT_CMD_ARGS;
}

void add_argument_to_command(command* cmd, word* arg) {
    if (cmd->length == cmd->reserved) {
        cmd->args = (word**) realloc(cmd->args, cmd->reserved * 2);
        cmd->reserved *= 2;
    }
    cmd->args[cmd->length] = arg;
    cmd->length++;
}

word* new_argument_to_command(command* cmd) {
    word* arg = (word*) malloc(1 * sizeof(word));
    initialize_word(arg);
    add_argument_to_command(cmd, arg);
    return arg;
}

void delete_command(command* cmd) {
    for (int i = 0; i < cmd->length; i++) {
        delete_word(cmd->args[i]);
    }
    free(cmd->args);
}

void print_command(command* cmd) {
    printf("\ncommand has %d arguments: ", cmd->length);
    for (int i = 0; i < cmd->length; i++) {
        printf("%s ", cmd->args[i]->data);
    }
    printf("\n");
}

void command_to_string(command* cmd, char** str) {
    int n_args = cmd->length;
    int total_length = 0;
    for (int i = 0; i < n_args; i++) {
        total_length += cmd->args[i]->length;
    }
    total_length += n_args - 1;
    *str = (char*) malloc((total_length + 1) * sizeof(char));
    int offset = 0;
    for (int i = 0; i < n_args; i++) {
        strcpy(*str + offset, cmd->args[i]->data);
        offset += cmd->args[i]->length;
        (*str)[offset++] = ' ';
    }
}

int parse_command(command* cmd) { // return 0 in case of correct argument, 1 if something went wrong (e.g. EOF)
    char c;    
    // --- initializing of state
    state st;
    initalize_state(&st);

    initialize_command(cmd);

    word* current_arg = NULL;

    // --- main loop
    int something_went_wrong = 0;
    while (1) {
        c = getchar();
        if (c == EOF) {
            something_went_wrong = 1;
            break;
        }        
        if (c == '\n') {
            break;
        }
        if (isspace(c)) {
            if (st.inside_quotes) {
                append_char_to_word(current_arg, c);
            } else {
                if (st.find_new_arg == 0) {
                    st.find_new_arg = 1;
                }
            }
        } else if (c == '"') {
            st.inside_quotes = (st.inside_quotes + 1) % 2;  // Мотяматяка
            if (st.find_new_arg == 1) {
                current_arg = new_argument_to_command(cmd);
                st.find_new_arg = 0;
            } else if (st.inside_quotes == 1) {
                perror("\nQoute found inside argument!\n");
                return 0;
            }
            append_char_to_word(current_arg, c);
        } else {
            if (st.find_new_arg == 1) {
                current_arg = new_argument_to_command(cmd);
                append_char_to_word(current_arg, c);
                st.find_new_arg = 0;
            } else {
                append_char_to_word(current_arg, c);
            }
        }
    }
    return something_went_wrong;
}

int main() {

    char path[MAX_PATH_LENGTH];

    while(1) {
        getcwd(path, MAX_PATH_LENGTH);
        printf(">>> %s: ", path);

        command cmd;
        int res = parse_command(&cmd);
        if (res) {
            // something went wrong
            delete_command(&cmd);
            break;
        }

        if ((cmd.length == 1) && (strcmp("exit", cmd.args[0]->data) ==0)) {
            break;
        }
        if ((cmd.length == 2) && (strcmp("cd", cmd.args[0]->data) ==0)) {
            int chdir_res = chdir(cmd.args[1]->data);
            if (chdir_res) {
                printf("--- no such directory\n");
            }
            continue;
        }

        // run this command
        char* s;
        command_to_string(&cmd, &s);
        int command_res = system(s);
        if (command_res) {
            printf("--- something went wrong: %s\n", strerror(errno));   
        }
        free(s);

        //print_command(&cmd);
        delete_command(&cmd);
    }

    return 0;
}

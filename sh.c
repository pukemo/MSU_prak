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

word* copy_word(word* w) {
    word* nw = (word*)malloc(sizeof(word));
    nw->length = w->length;
    nw->reserved = w->reserved;
    nw->data = (char*)malloc(w->length * sizeof(char));
    strcpy(nw->data, w->data);
    return nw;
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

void print_command(command* cmd) {
    printf("command has %d arguments: ", cmd->length);
    for (int i = 0; i < cmd->length; i++) {
        printf("%s ", cmd->args[i]->data);
    }
    printf("\n");
    fflush(stdout);
}

void add_argument_to_command(command* cmd, word* arg) {
    if (cmd->length == cmd->reserved) {
        cmd->args = (word**) realloc(cmd->args, (cmd->reserved * 2) * sizeof(word*));
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

void command_to_string_from_to(command* cmd, char** str, int start, int end) {
    int total_length = 0;
    for (int i = start; i < end; i++) {
        total_length += cmd->args[i]->length;
    }
    total_length += (end - start) - 1;
    *str = (char*) malloc((total_length + 1) * sizeof(char));
    int offset = 0;
    for (int i = start; i < end; i++) {
        strcpy(*str + offset, cmd->args[i]->data);
        offset += cmd->args[i]->length;
        if (i < end - 1)
            (*str)[offset++] = ' ';
    }
}

void command_to_string(command* cmd, char** str) {
    command_to_string_from_to(cmd, str, 0, cmd->length);
}

int parse_command(command* cmd) { // return 0 in case of correct argument, 1 if something went wrong (e.g. EOF)
    char c;    
    // --- initializing of state
    state st;
    initalize_state(&st);

    initialize_command(cmd);

    word* current_arg = NULL;

    // --- parse command loop ---
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

int is_redirection(word* arg) {
    // returns:     0 if arg is not IO redirection,
    //              1 if '<' ,
    //              2 if '>' ,
    //              3 if '>>' ,
    if (strcmp("<", arg->data) == 0) {
        return 1;
    }
    if (strcmp(">", arg->data) == 0) {
        return 2;
    }
    if (strcmp(">>", arg->data) == 0) {
        return 3;
    }
    return 0;
}

int is_pipe(word* arg) {
    return (strcmp("|", arg->data) == 0);
}

int has_io_redirection(command* cmd, char** command_str, char** file_name) {
    // returns:     0 if no IO redirection found,
    //              1 if '<' found, file_name updated,
    //              2 if '>' found, file_name updated,
    //              3 if '>>' found, file_name updated,
    //              -1 if command is incorrect
    *command_str = NULL;
    *file_name = NULL;
    int redirection_count = 0;
    for (int i = 0; i < cmd->length; i++) {
        redirection_count += (int)(is_redirection(cmd->args[i]) != 0);
    }
    if (redirection_count > 1) {
        return -1;
    }
    if (redirection_count == 0) {
        return 0;
    }
    int ioRed = is_redirection(cmd->args[cmd->length - 2]);
    if (ioRed == 0) {
        return -1;
    }
    // if (cmd->length != 3) {
    //     return -1;
    // }
    *file_name = cmd->args[cmd->length - 1]->data;
    command_to_string_from_to(cmd, command_str, 0, cmd->length - 2);
    return ioRed;
}

int check_pipes_correctness(command* cmd) {
    if (is_pipe(cmd->args[0]) || is_pipe(cmd->args[cmd->length-1])) {
        return -1;
    }
    for (int i = 1; i < cmd->length; i++) {
        if (is_pipe(cmd->args[i - 1]) && is_pipe(cmd->args[i])) {
            return -1;
        }
    }
    return 0;
}

int pipes_count(command* cmd) {
    int count = 0;
    for (int i = 0; i < cmd->length; i++) {
        if (is_pipe(cmd->args[i]))
            count++;
    }
    return count;
}

int split_command_through_pipeline(command* cmd_src, command*** cmds, int* length) {
    if (check_pipes_correctness(cmd_src)) {
        return -1;
    }
    int n = pipes_count(cmd_src);
    *cmds = (command**)malloc((n + 1) * sizeof(command*));
    for (int i = 0; i < n + 1; i++) {
        (*cmds)[i] = (command*)malloc(sizeof(command));
        initialize_command((*cmds)[i]);
    }

    int j = 0;
    for (int i = 0; i < cmd_src->length; i++) {
        word* argument = cmd_src->args[i];
        if (!is_pipe(argument)) {
            word* new_argument = copy_word(argument);
            add_argument_to_command((*cmds)[j], new_argument);
        } else {
            j++;
        }
    }
    *length = n + 1;
    return 0;
}

void delete_pipeline(command** pipeline, int length) {
    for (int i = 0; i < length; i++) {
        delete_command(pipeline[i]);
    }
    free(pipeline);
}

void print_pipeline(command** cmds, int length) {
    printf("pipeline with %d commands:\n", length);
    for (int i = 0; i < length; i++) {
        printf("%d) ", i + 1);
        print_command(cmds[i]);
    }
}

void print_current_path() {
    char path[MAX_PATH_LENGTH];
    getcwd(path, MAX_PATH_LENGTH);
    printf(">>> %s: ", path);
}

void execute_command_str(char* s) {
    int command_res = system(s);
    if (command_res) {
        printf("--- something went wrong: %s\n", strerror(errno));   
    }
}

void execute_command_simple(command* cmd) {
    char* s;
    command_to_string(cmd, &s);
    execute_command_str(s);
    free(s);
}

void custom_command_cd(command* cmd) {
    int chdir_res = chdir(cmd->args[1]->data);
    if (chdir_res) {
        printf("--- no such directory\n");
    }
}

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

void execute_command_with_redirection(int redirection, char* command_str, char* file_name) {
    if (redirection == -1) {
        printf("ERROR IN REDIRECTION SYNTAX\n");
    } else {
        int fd;
        fpos_t pos;
        FILE* stream;
        char* mode;
        if (redirection == 1) {
            printf("redirection <: from file '%s' to '%s'\n", file_name, command_str);
            stream = stdin;
            mode = "r";
        } else if (redirection == 2) {
            printf("redirection >: '%s' to file '%s'\n", command_str, file_name);
            stream = stdout;
            mode = "w";
        } else if (redirection == 3) {
            printf("redirection >>: '%s' to file '%s'\n", command_str, file_name);
            stream = stdout;
            mode = "a"; //append
        }
        redirect_stream_to_file(stream, file_name, mode, &fd, &pos);
        execute_command_str(command_str);
        restore_stream(stream, fd, pos);
    }
}

int execute_command(command* cmd) {
    int exit = 0;
    char* file_name;
    char* command_str;
    int t = has_io_redirection(cmd, &command_str, &file_name);
    if (t) {
        // ---- IO redirection found ----
        execute_command_with_redirection(t, command_str, file_name);
        if (command_str)
            free(command_str);
    } else {
        // --- no IO redirection found ---
        if ((cmd->length == 1) && (strcmp("exit", cmd->args[0]->data) ==0)) {
            exit = 1;
        }
        if ((cmd->length == 2) && (strcmp("cd", cmd->args[0]->data) ==0)) {
            custom_command_cd(cmd);
        } else {
            // run this command
            execute_command_simple(cmd);
        }
    }
    return exit;
}

int execute_pipeline(command** pipeline, int length) {
    int n_pipes = length - 1;
    int* fd = (int*)malloc(n_pipes * sizeof(int) * 2);
    for (int i = 0; i < n_pipes; i++) {
        pipe(&(fd[2 * i]));
    }
    for (int i = 0; i < length; i++) {
        if (fork()) {
            // parent
            printf("parent at iteration %d\n", i);
        } else {
            // child
            printf("%d: ", getpid());
            print_command(pipeline[i]);
            if (i > 1) {
                dup2(fd[2 * (i - 1)], fileno(stdin)); // redirect input
                close(fd[2 * (i - 1)]);
            }
            if (i < length - 1)
                dup2(fd[2 * i + 1], fileno(stdout)); // redirect output
            
            printf("%d. in: %d, out: %d\n", getpid(), fd[2 * (i - 1)], fd[2 * i + 1]);
            execute_command(pipeline[i]);
            
            if (i < length - 1)
                close(fd[2 * i + 1]);
            exit(0);
        }
    }
    free(fd);
    return 0;
}


int execute_pipeline2(command* cmd1, command* cmd2) {
    int fd[2];
    pipe(fd);
    if (fork()) {
        if (fork()) {
            // second child
            dup2(fd[1], fileno(stdout));
            close(fd[0]);
            // execvp(cmd1->args[0], cmd1->args);
            execute_command_simple(cmd1);
            close(fd[1]);
            exit(0);
        } else {
            // parent
        }
    } else {
        // child
        dup2(fd[0], fileno(stdin));
        // execvp(cmd2->args[0], cmd2->args);
        execute_command_simple(cmd2);
        close(fd[0]);
        close(fd[1]);
        exit(0);
    }
    return 0;
}

int main() {
    // main shell loop
    int exit = 0;
    while(!exit) {
        print_current_path();
        command cmd;
        int res = parse_command(&cmd);
        if (res) {
            // something went wrong
            printf("SOMETHING WENT WRONG");
            delete_command(&cmd);
            break;
        }
        if (cmd.length > 0) {
            command** pipeline;
            int length;
            int r = split_command_through_pipeline(&cmd, &pipeline, &length);
            if (r) {
                printf("ERROR IN PIPELINE SYNTAX\n");
                delete_command(&cmd);
                continue;
            }
            if (length > 1) {
                // print_pipeline(pipeline, length);
                // execute_pipeline(pipeline, length);
                exit = execute_pipeline2(pipeline[0], pipeline[1]);
            } else {
                exit = execute_command(&cmd);
            }
            delete_pipeline(pipeline, length);
        }
        
        delete_command(&cmd);
    }

    return 0;
}

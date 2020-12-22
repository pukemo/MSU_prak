#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define DEFAULT_WORD_LENGTH 4
#define DEFAULT_CMD_ARGS 2
#define MAX_PATH_LENGTH 256

enum {
    PIPE,        // |
    DOT,         // ;
    DOUBLE_AMP,  // &&
    DOUBLE_PIPE  // ||
};

#define DEBUG

typedef struct state {
    int find_new_arg;
    int inside_quotes;
    int amp;
    int wertical;
    int point;
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
    int bg;
    char* filename;
    int redirection_state;
    int in;
    int out;
} command;

typedef struct NODE {
    pid_t data;
    struct NODE* next;
} NODE;

void list_add(NODE** head, pid_t data) {
    NODE* temp = (NODE*)malloc(sizeof(NODE));
    temp->data = data;
    temp->next = (*head);
    (*head) = temp;
}

int list_remove(NODE** head) {
    if (!head) return -1;
    if (!*head) return -2;
    NODE* tmp = (*head)->next;
    free(*head);
    *head = tmp;
    return 0;
}


void initalize_state(state* st) {
    st->find_new_arg = 1;
    st->inside_quotes = 0;
    st->amp = 0;
    st->wertical = 0;
    st->point = 0;
}

void initialize_word(word* w) {
    w->data = (char*)malloc(DEFAULT_WORD_LENGTH * sizeof(char));
    w->data[0] = '\0';
    w->length = 0;
    w->reserved = DEFAULT_WORD_LENGTH;
}

void append_char_to_word(word* w, char c) {
    if (w->length == w->reserved - 1) {
        w->data = (char*)realloc(w->data, w->reserved * 2);
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
    nw->data = (char*)malloc((w->length + 1) * sizeof(char));
    strcpy(nw->data, w->data);
    return nw;
}

void delete_word(word* w) {
    if (!w) return;
    if (w->data) free(w->data);
    w->data = NULL;
    free(w);
}

void initialize_command(command* cmd) {
    cmd->args = (word**)malloc(DEFAULT_CMD_ARGS * sizeof(word*));
    cmd->length = 0;
    cmd->reserved = DEFAULT_CMD_ARGS;
    cmd->bg = 0;
    cmd->filename = NULL;
    cmd->redirection_state = 0;
    cmd->in = dup(0);
    cmd->out = dup(1);
}

void print_command(command* cmd) {
    printf("command has %d arguments: ", cmd->length);
    fflush(stdout);
    for (int i = 0; i < cmd->length; i++) {
        printf("%s ", cmd->args[i]->data);
        fflush(stdout);
    }
    printf("\n");
    fflush(stdout);
}

void add_argument_to_command(command* cmd, word* arg) {
    if (cmd->length == cmd->reserved) {
        cmd->args = (word**)realloc(cmd->args, (cmd->reserved * 2) * sizeof(word*));
        cmd->reserved *= 2;
    }
    cmd->args[cmd->length] = arg;
    cmd->length++;
}

word* new_argument_to_command(command* cmd) {
    word* arg = (word*)malloc(1 * sizeof(word));
    initialize_word(arg);
    add_argument_to_command(cmd, arg);
    return arg;
}

void add_last_to_command(command* cmd) {
    append_char_to_word(new_argument_to_command(cmd), '\0');
}

int delete_last_of_command(command* cmd) {
    if (cmd->length < 2) return 1;
    if (cmd->args[cmd->length - 1]) {
        delete_word(cmd->args[cmd->length - 1]);
        cmd->args[cmd->length - 1] = NULL;
    }
    if (cmd->args[cmd->length - 2]) {
        delete_word(cmd->args[cmd->length - 1]);
        cmd->args[cmd->length - 2] = NULL;
    }
    cmd->length -= 2;
    add_last_to_command(cmd);
    return 0;
}

void delete_command(command* cmd) {
    //printf("DELETING COMMAND\n    ");
    //print_command(cmd);
    //fflush(stdout);
    if (!cmd) return;
    for (int i = 0; i < cmd->length; i++) {
        //if(cmd->args[i]) printf("  DEL %d %s\n", i, cmd->args[i]->data);
        //fflush(stdout);
        if (cmd->args[i]) delete_word(cmd->args[i]);
        cmd->args[i] = NULL;
    }
    if (cmd->args) {
        free(cmd->args);
        cmd->args = NULL;
    }
    if (cmd->filename) {
        free(cmd->filename);
        cmd->filename = NULL;
    }
    close(cmd->in);
    close(cmd->out);
}

void command_to_string_from_to(command* cmd, char** str, int start, int end) {
    int total_length = 0;
    for (int i = start; i < end; i++) {
        total_length += cmd->args[i]->length;
    }
    total_length += (end - start) - 1;
    *str = (char*)malloc((total_length + 1) * sizeof(char));
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
            }
            else {
                if (st.find_new_arg == 0) {
                    st.find_new_arg = 1;
                }
                st.amp = 0;
                st.wertical = 0;
                st.point = 0;
            }
        }
        else if (c == '&') {
            if (st.inside_quotes) {
                append_char_to_word(current_arg, c);
            }
            else if (st.wertical || st.point) {
                perror("\nInvalid combination of ';', '&' and pipe symbols!\n");
                return 0;
            }
            else {
                st.amp++;
                if (st.amp == 1) {
                    current_arg = new_argument_to_command(cmd);
                    append_char_to_word(current_arg, c);
                }
                else if (st.amp == 2) {
                    append_char_to_word(current_arg, c);
                    st.find_new_arg = 1;
                }
                else {
                    perror("\nToo many '&' symbols!\n");
                    return 0;
                }
            }
            st.wertical = 0;
            st.point = 0;
        }
        else if (c == '|') {
            if (st.inside_quotes) {
                append_char_to_word(current_arg, c);
            }
            else if (st.amp > 1 || st.point) {
                perror("\nInvalid combination of ';', '&' and pipe symbols!\n");
                return 0;
            }
            else {
                st.wertical++;
                if (st.wertical == 1) {
                    current_arg = new_argument_to_command(cmd);
                    append_char_to_word(current_arg, c);
                }
                else if (st.wertical == 2) {
                    append_char_to_word(current_arg, c);
                    st.find_new_arg = 1;
                }
                else {
                    perror("\nToo many pipe symbols!\n");
                    return 0;
                }
            }
            st.amp = 0;
            st.point = 0;
        }
        else if (c == ';') {
            if (st.inside_quotes) {
                append_char_to_word(current_arg, c);
            }
            else if (st.amp > 1 || st.wertical) {
                perror("\nInvalid combination of ';', '&' and pipe symbols!\n");
                return 0;
            }
            else {
                st.point++;
                if (st.point == 1) {
                    current_arg = new_argument_to_command(cmd);
                    append_char_to_word(current_arg, c);
                    st.find_new_arg = 1;
                }
                else {
                    perror("\nToo many ';' symbols!\n");
                    return 0;
                }
            }
            st.amp = 0;
            st.wertical = 0;
        }
        else if (c == '"') {
            st.inside_quotes = (st.inside_quotes + 1) % 2;  // Мотяматяка
            if (st.find_new_arg == 1) {
                current_arg = new_argument_to_command(cmd);
                st.find_new_arg = 0;
            }
            else if (st.inside_quotes == 1) {
                perror("\nQuote found inside argument!\n");
                return 0;
            }
            append_char_to_word(current_arg, c);
            st.amp = 0;
            st.wertical = 0;
            st.point = 0;
        }
        else {
            if (st.find_new_arg == 1 || st.amp || st.wertical || st.point) {
                current_arg = new_argument_to_command(cmd);
                append_char_to_word(current_arg, c);
                st.find_new_arg = 0;
                st.amp = 0;
                st.wertical = 0;
                st.point = 0;
            }
            else {
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

int check_command_bg(command* cmd) {
    // return 0 if not bg command, 1 if bg command, -1 if incorrect bg command
    int cnt = 0;
    int amp_pos = -1;
    for (int i = 0; i < cmd->length; i++) {
        if (!(strcmp(cmd->args[i]->data, "&"))) {
            cnt++;
            amp_pos = i;
        }
    }
    if (cnt > 1) {
        return -1;
    }
    if (cnt == 1 && amp_pos != cmd->length - 2) {
        return -1;
    }
    if (cnt == 1) {
        cmd->bg = 1;
        return 1;
    }
    return 0;
}

int is_bg_or_redirection(command* cmd) { //returns -1 if syntax error
    int red_flag = 0;
    if (cmd->length < 2) return -1;
    if (is_redirection(cmd->args[cmd->length - 2])) return -1;
    if (cmd->length >= 3) {
        cmd->redirection_state = is_redirection(cmd->args[cmd->length - 3]);
        if (cmd->redirection_state) {
            if (cmd->length == 3) return -1;
            cmd->filename = (char*)malloc(cmd->args[cmd->length - 2]->length * sizeof(char));
            strcpy(cmd->filename, cmd->args[cmd->length - 2]->data);
            delete_last_of_command(cmd);
            delete_last_of_command(cmd);
            red_flag = 1;
        }
    }
    cmd->bg = check_command_bg(cmd);
    if (cmd->bg == -1 || (cmd->bg == 1 && cmd->length == 2)) return -1;
    if (cmd->bg) delete_last_of_command(cmd);
    int i;
    for (i = 0; i < cmd->length - 3; i++) {
        if (is_redirection(cmd->args[i])) return -1;
    }
    if (is_redirection(cmd->args[cmd->length - 2])) return -1;
    if (cmd->length >= 3) {
        cmd->redirection_state = is_redirection(cmd->args[cmd->length - 3]);
        if (cmd->redirection_state) {
            if (red_flag || cmd->length == 3)  return -1;
            cmd->filename = (char*)malloc(cmd->args[cmd->length - 2]->length * sizeof(char));
            strcpy(cmd->filename, cmd->args[cmd->length - 2]->data);
            delete_last_of_command(cmd);
            delete_last_of_command(cmd);
        }
    }
    return 0;
}

void delete_command_arr(command** cmds, int length) {
    if (!cmds) return;
    if (!length) {
        free(cmds);
        return;
    }
    int i;
    for (i = 0; i < length; i++) {
        delete_command(cmds[i]);
        if (cmds[i]) {
            free(cmds[i]);
            cmds[i] = NULL;
        }
    }
    free(cmds);
}

int split_to_operations(command* cmd, command*** cmds, int** operations) { //returns length of cmds or -1 if error
    int i, j = -1; //j - length of operations
    int k = -1; //length of cmds
    command* cur_cmd = (command*)malloc(sizeof(command));
    initialize_command(cur_cmd);
    *cmds = (command**)malloc(sizeof(command*));
    for (i = 0; i < cmd->length; i++) {
        if (!(strcmp(cmd->args[i]->data, "|"))) {
            if (k == -1 && !(cur_cmd->length)) {
                delete_command(cur_cmd);
                if (cur_cmd) {
                    free(cur_cmd);
                    cur_cmd = NULL;
                }
                delete_command_arr(*cmds, 0);
                *cmds = NULL;
                return -1;
            }
            add_last_to_command(cur_cmd);
            if (is_bg_or_redirection(cur_cmd)) {
                delete_command(cur_cmd);
                cur_cmd = NULL;
                if (cur_cmd) {
                    free(cur_cmd);
                    cur_cmd = NULL;
                }
                delete_command_arr(*cmds, k + 1);
                *cmds = NULL;
                return -1;
            }
            *cmds = (command**)realloc(*cmds, sizeof(command*) * (k + 2));
            (*cmds)[k + 1] = cur_cmd;
            cur_cmd = (command*)malloc(sizeof(command));
            initialize_command(cur_cmd);
            k++;
            if (j < 0) {
                *operations = (int*)malloc(sizeof(int));
                j = 0;
            }
            else {
                *operations = (int*)realloc(*operations, sizeof(int) * (j + 2));
            }
            (*operations)[j] = PIPE;
            j++;
        }
        else if (!(strcmp(cmd->args[i]->data, ";"))) {
            if (k == -1 && !(cur_cmd->length)) {
                delete_command(cur_cmd);
                if (cur_cmd) {
                    free(cur_cmd);
                    cur_cmd = NULL;
                }
                delete_command_arr(*cmds, 0);
                *cmds = NULL;
                return -1;
            }
            add_last_to_command(cur_cmd);
            if (is_bg_or_redirection(cur_cmd)) {
                delete_command(cur_cmd);
                cur_cmd = NULL;
                if (cur_cmd) {
                    free(cur_cmd);
                    cur_cmd = NULL;
                }
                delete_command_arr(*cmds, k + 1);
                *cmds = NULL;
                return -1;
            }
            *cmds = (command**)realloc(*cmds, sizeof(command*) * (k + 2));
            (*cmds)[k + 1] = cur_cmd;
            cur_cmd = (command*)malloc(sizeof(command));
            initialize_command(cur_cmd);
            k++;

            if (j < 0) {
                *operations = (int*)malloc(sizeof(int));
                j = 0;
            }
            else {
                *operations = (int*)realloc(*operations, sizeof(int) * (j + 2));
            }
            (*operations)[j] = DOT;
            j++;
        }
        else if (!(strcmp(cmd->args[i]->data, "&&"))) {
            if (k == -1 && !(cur_cmd->length)) {
                delete_command(cur_cmd);
                if (cur_cmd) {
                    free(cur_cmd);
                    cur_cmd = NULL;
                }
                delete_command_arr(*cmds, 0);
                *cmds = NULL;
                return -1;
            }
            add_last_to_command(cur_cmd);
            if (is_bg_or_redirection(cur_cmd)) {
                delete_command(cur_cmd);
                cur_cmd = NULL;
                if (cur_cmd) {
                    free(cur_cmd);
                    cur_cmd = NULL;
                }
                delete_command_arr(*cmds, k + 1);
                *cmds = NULL;
                return -1;
            }
            *cmds = (command**)realloc(*cmds, sizeof(command*) * (k + 2));
            (*cmds)[k + 1] = cur_cmd;
            cur_cmd = (command*)malloc(sizeof(command));
            initialize_command(cur_cmd);
            k++;
            if (j < 0) {
                *operations = (int*)malloc(sizeof(int));
                j = 0;
            }
            else {
                *operations = (int*)realloc(*operations, sizeof(int) * (j + 2));
            }
            (*operations)[j] = DOUBLE_AMP;
            j++;
        }
        else if (!(strcmp(cmd->args[i]->data, "||"))) {
            if (k == -1 && !(cur_cmd->length)) {
                delete_command(cur_cmd);
                if (cur_cmd) {
                    free(cur_cmd);
                    cur_cmd = NULL;
                }
                delete_command_arr(*cmds, 0);
                *cmds = NULL;
                return -1;
            }
            add_last_to_command(cur_cmd);
            if (is_bg_or_redirection(cur_cmd)) {
                delete_command(cur_cmd);
                cur_cmd = NULL;
                if (cur_cmd) {
                    free(cur_cmd);
                    cur_cmd = NULL;
                }
                delete_command_arr(*cmds, k + 1);
                *cmds = NULL;
                return -1;
            }
            *cmds = (command**)realloc(*cmds, sizeof(command*) * (k + 2));
            (*cmds)[k + 1] = cur_cmd;
            cur_cmd = (command*)malloc(sizeof(command));
            initialize_command(cur_cmd);
            k++;
            if (j < 0) {
                *operations = (int*)malloc(sizeof(int));
                j = 0;
            }
            else {
                *operations = (int*)realloc(*operations, sizeof(int) * (j + 2));
            }
            (*operations)[j] = DOUBLE_PIPE;
            j++;
        }
        else {
            add_argument_to_command(cur_cmd, copy_word(cmd->args[i]));
        }
    }
    if (!(cur_cmd->length)) {
        if (k == -1) return 0;
        delete_command(cur_cmd);
        if (cur_cmd) {
            free(cur_cmd);
            cur_cmd = NULL;
        }
        delete_command_arr(*cmds, k + 1);
        *cmds = NULL;
        if (*operations) {
            free(*operations);
            *operations = NULL;
        }
        return -1;
    }
    add_last_to_command(cur_cmd);
    if (is_bg_or_redirection(cur_cmd)) {
        delete_command(cur_cmd);
        if (cur_cmd) {
            free(cur_cmd);
            cur_cmd = NULL;
        }
        delete_command_arr(*cmds, k + 1);
        *cmds = NULL;
        if (*operations) {
            free(*operations);
            *operations = NULL;
        }
        return -1;
    }
    *cmds = (command**)realloc(*cmds, sizeof(command*) * (k + 2));
    (*cmds)[k + 1] = cur_cmd;
    return (k + 2);
}

void print_current_path() {
    char path[MAX_PATH_LENGTH];
    getcwd(path, MAX_PATH_LENGTH);
    printf(">>> %s: ", path);
    fflush(stdout);
}

void see_current_path() {
    char path[MAX_PATH_LENGTH];
    getcwd(path, MAX_PATH_LENGTH);
    printf("Current PATH: %s\n", path);
    fflush(stdout);
}

void print_info_path() {
    printf("~To see current $PATH enter word PATH\n");
    printf("~To add directory to $PATH enter this:\n        add_PATH *path to adding directory*\n");
    printf("  ~Example of adding directory:\n        add_PATH /opt/local/bin\n");
    printf("~To see this information again enter word info\n");
}

void add_path(char* path) {
    char arg[MAX_PATH_LENGTH + 20];
    sprintf(arg, "export PATH=$PATH:%s\n", path);
    system(arg);
}

const char** command_st(command* cmd, int remove_last) {
    int n = cmd->length;
    if (remove_last) {
        n--;
    }
    char** args = (char**)malloc(n * sizeof(char*));
    for (int i = 0; i < n - 1; i++) {
        args[i] = cmd->args[i]->data;
    }
    args[n - 1] = NULL;
    return (const char**)args;
}

int custom_command_cd(command* cmd) {
    int chdir_res = chdir(cmd->args[1]->data);
    if (chdir_res) {
        printf("--- no such directory\n");
        fflush(stdout);
        return 1;
    }
    return 0;
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

void bg_wait(NODE** bg_pids, int opt) {
    NODE* pid;
    while (*bg_pids) {
        int status = 0;
        pid = *bg_pids;
        waitpid(pid->data, &status, opt);
        if (WIFEXITED(status)) {
#ifdef DEBUG
            printf("BG JOB %d FINISHED WITH STATUS %d\n", pid->data, WEXITSTATUS(status));
            fflush(stdout);
#endif
        }
        list_remove(bg_pids);
    }
}

void non_bg_wait(NODE** bg_pids, int opt) {
    NODE* pid;
    while (*bg_pids) {
        int status = 0;
        pid = *bg_pids;
        waitpid(pid->data, &status, opt);
        list_remove(bg_pids);
    }
}

int execute_command(command*** cmds, int place, NODE** bg_pids, NODE** not_bg_pids, int length, int** operations, pid_t* last_pid) {
    //printf("EXECUTING COMMAND %d\n    ", place);
    //print_command((*cmds)[place]);
    //fflush(stdout);
    pid_t pid = 0;
    int fd[2];
    int exit_flag = 0;
    command* cmd = (*cmds)[place];
    command* next_cmd;
    if ((cmd->length == 2) && (strcmp("exit", cmd->args[0]->data) == 0)) {
        exit_flag = 1;
    }
    int bg = cmd->bg;
    int red = cmd->redirection_state;
    if ((!red || red == 1) && (place + 1) < length && (*operations)[place] == PIPE) {
        if (pipe(fd)) {
            *last_pid = -1;
            return exit_flag;
        }
        next_cmd = (*cmds)[place + 1];
        if (dup2(fd[0], next_cmd->in) == -1) {
            *last_pid = -1;
            close(fd[0]);
            close(fd[1]);
            return exit_flag;
        }
        if (dup2(fd[1], cmd->out) == -1) {
            *last_pid = -1;
            close(fd[0]);
            close(fd[1]);
            return exit_flag;
        }
        close(fd[0]);
        close(fd[1]);
    }
    if ((cmd->in && (red == 2 || red == 3)) || (cmd->out != 1 && red == 1)) {
        printf("SYNTAX ERROR\n");
        fflush(stdout);
        *last_pid = -1;
        return exit_flag;
    }
    int fd_file;
    fpos_t pos;
    char* mode;
    FILE* stream;
    char* command_string;
    command_to_string(cmd, &command_string);
    switch (red) {
    case(1):
        printf("redirection <: from file '%s' to '%s'\n", cmd->filename, command_string);
        fflush(stdout);
        stream = stdin;
        mode = (char*)"r";
        break;
    case(2):
        printf("redirection >: '%s' to file '%s'\n", command_string, cmd->filename);
        fflush(stdout);
        stream = stdout;
        mode = (char*)"w";
        break;
    case(3):
        printf("redirection >>: '%s' to file '%s'\n", command_string, cmd->filename);
        fflush(stdout);
        stream = stdout;
        mode = (char*)"a";
    default:
        break;
    }
    if (command_string) {
        free(command_string);
        command_string = NULL;
    }
    int status = 0;
    int option = 1;
    if (red) redirect_stream_to_file(stream, cmd->filename, mode, &fd_file, &pos);
    if (place && (*operations)[place - 1]) {
        if (*last_pid < (pid_t)2) {
            printf("ERROR waiting for the process with the wrong pid\n");
            fflush(stdout);
            return -3;
        }
        switch ((*operations)[place - 1]) {
        case(DOT):
            // operation ;;
            waitpid(*last_pid, NULL, 0);
            break;
        case(DOUBLE_AMP):
            // operation &&
            waitpid(*last_pid, &status, 0);
            if (!WIFEXITED(status) || (WIFEXITED(status) && WEXITSTATUS(status))) option = 0;
            break;
        case(DOUBLE_PIPE):
            // operation ||
            waitpid(*last_pid, &status, 0);
            if (WIFEXITED(status) && !WEXITSTATUS(status)) option = 0;
        default:
            break;
        }
    }
    if (!(pid = fork())) {
        // son
        fflush(stdout);

        if (!option) {
            close(cmd->in);
            close(cmd->out);
            exit(EXIT_FAILURE);
        }
        if (bg) {
#ifdef DEBUG
            char* s;
            command_to_string(cmd, &s);
            printf("RUNNING BG JOB '%s' in process %d\n", s, getpid());
            fflush(stdout);
            free(s);
#endif
        }

        switch (red) {
        case(0):
            // no redirection
            dup2(cmd->in, 0);
        case(1):
            // redirection <
            dup2(cmd->out, 1);
            break;
        case(2):
            // redirection >
        case(3):
            // redirection >>
            dup2(cmd->in, 0);
        default:
            break;
        }
        close(cmd->in);
        close(cmd->out);
        fflush(stdout);
        if (cmd->length == 2) {
            if (strcmp("exit", cmd->args[0]->data) == 0) {
                exit(0);
            }
            else if (strcmp("PATH", cmd->args[0]->data) == 0) {
                see_current_path();
                exit(0);
            }
            else if (strcmp("info", cmd->args[0]->data) == 0) {
                print_info_path();
                exit(0);
            }
        }
        if (cmd->length == 3) {
            if (strcmp("cd", cmd->args[0]->data) == 0) {
                if (custom_command_cd(cmd)) exit(EXIT_FAILURE);
                exit(0);
            }
            else if (strcmp("add_PATH", cmd->args[0]->data) == 0) {
                add_path(cmd->args[1]->data);
                exit(0);
            }
        }
        const char** command_str = command_st(cmd, 0);
        execvp(command_str[0], (char* const*)command_str);
        printf("ERROR while executing\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    // father
    close(cmd->out);
    close(cmd->in);
    fflush(stdout);
    if (pid > 1) {
        if (bg) {
            list_add(bg_pids, pid);
        }
        else {
            list_add(not_bg_pids, pid);
        }
    }
    if (red) restore_stream(stream, fd_file, pos);
    *last_pid = pid;
    if (option && pid < 2) printf("ERORR WITH CREATING A CHILD\n");
    fflush(stdout);
    return exit_flag;
}

int main() {
    // main shell loop
    printf("SHELL STARTED\n");
    print_info_path();
    int exit = 0;
    NODE* bg_pids = NULL;
    NODE* not_bg_pids = NULL;
    command cmd;
    int res;
    while (!exit) {
        print_current_path();
        res = parse_command(&cmd);
        //print_command(&cmd);
        //fflush(stdout);
        if (res) {
            // something went wrong
            printf("SOMETHING WENT WRONG: EOF symbol\n");
            fflush(stdout);
            delete_command(&cmd);
            break;
        }
        if (cmd.length > 0) {
            command** cmds = NULL;
            int* operations = NULL;
            int length = split_to_operations(&cmd, &cmds, &operations);
            if (length < 0) {
                printf("SYNTAX ERROR\n");
                fflush(stdout);
                delete_command(&cmd);
                continue;
            }
            if (!length) continue;
            int i;
            pid_t last_pid = -1;
            for (i = 0; i < length; i++) {
                exit = execute_command(&cmds, i, &bg_pids, &not_bg_pids, length, &operations, &last_pid);
                if (last_pid < 0) {
                    printf("AN ERROR OCCURED\n");
                    fflush(stdout);
                    break;
                }
                if (exit) break;
                if (!last_pid) break;
            }
            delete_command_arr(cmds, length);
            cmds = NULL;
            if (operations) {
                free(operations);
                operations = NULL;
            }
            delete_command(&cmd);
            non_bg_wait(&not_bg_pids, 0);
        }
    }
    bg_wait(&bg_pids, 0);
    bg_wait(&not_bg_pids, 0);
    printf("SHELL ENDED");
    return 0;
}

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

#define DEBUG

typedef struct state {
    int find_new_arg;
    int inside_quotes;
    int amp;
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
    int pid;
    word* filename;
    int redirection_state;
    int in;
    int out;
} command;

typedef struct NODE {
    pid_t data;
    struct NODE* next;
} NODE;


void list_print(NODE* head) {
    while (head) {
        printf("%d ", head->data);
        head = head->next;
    }
    printf("\n");
}

void list_add(NODE** head, pid_t data) {
    NODE* temp = (NODE*) malloc(sizeof (NODE));
    temp->data = data;
    temp->next = (*head);
    (*head) = temp;
}

int list_find(NODE* head, pid_t data) {
    while (head) {
        if (head->data == data)
            return 1;
        head = head->next;
    }
    return 0;
}

int list_empty(NODE* head) {
    return head == NULL;
}

void list_remove(NODE** head, pid_t data) {
    if ((*head)->data == data) {
        NODE* tmp = (*head)->next;
        free(*head);
        (*head) = (*head)->next;
    } else {
        NODE* tmp = *head;
        while (tmp->next->data != data) {
          tmp = tmp->next;
        }
        NODE* t = tmp->next->next;
        free(tmp->next);
        tmp->next = t;
    }
}


void initalize_state(state* st) {
    st->find_new_arg = 1;
    st->inside_quotes = 0;
    st->amp = 0;
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
    cmd->bg = 0;
    cmd->pid = -2;
    cmd->filename = NULL;
    cmd->redirection_state = 0;
    cmd->in = 0;
    cmd->out = 1;
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

void add_last_to_command(command* cmd) {
    strcpy(new_argument_to_command->data, "\0");
}

int delete_last_of_command(command* cmd) {
    if (cmd->length < 2) return 1;
    free(cmd->args[cmd->length - 2]);
    free(cmd->args[cmd->length - 2]);
    add_last_to_command(cmd);
    cmd->length -= 2;
    return 0;
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
                st.amp = 0;
            }
        } else if (c == '&') {
            if (st.inside_quotes) {
                append_char_to_word(current_arg, c);
            } else {
                st.amp++;
                if (st.amp == 1) {
                    current_arg = new_argument_to_command(cmd);
                    append_char_to_word(current_arg, c);
                } else if (st.amp == 2) {
                    append_char_to_word(current_arg, c);
                    st.find_new_arg = 1;
                } else {
                    perror("\nToo many amps!\n");
                    return 0;
                }
            }
        } else if (c == '"') {
            st.inside_quotes = (st.inside_quotes + 1) % 2;  // Мотяматяка
            if (st.find_new_arg == 1) {
                current_arg = new_argument_to_command(cmd);
                st.find_new_arg = 0;
            } else if (st.inside_quotes == 1) {
                perror("\nQuote found inside argument!\n");
                return 0;
            }
            append_char_to_word(current_arg, c);
            st.amp = 0;
        } else {
            if (st.amp > 0) {
                perror("\nAmp found inside argument!\n");
                return 0;
            }
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

int check_command_bg(command* cmd) {
    // return 0 if not bg command, 1 if bg command, -1 if incorrect bg command
    int cnt = 0;
    int amp_pos = -1;
    for (int i = 0; i < cmd->length; i++) {
        if (is_amp(cmd->args[i])) {
            cnt++;
            amp_pos = i;
        }
    }
    if (cnt > 1) {
        return -1;
    }
    if (cnt == 1 && amp_pos != cmd->length - 1) {
        return -1;
    }
    if (cnt == 1) {
        cmd->bg = 1;
        return 1;
    }
    return 0;
}

void is_bg_or_redirection(command* cmd) {
    cmd->bg = check_command_bg(cmd);
    if (cmd->bg == -1) return;

}

int split_to_operations(command* cmd, command*** cmds, int** operations) { //returns length of cmds or -1 if error
    //operation numbers:
    //0 - |
    //1 - ;
    //2 - &&
    //3 - ||
    int i, j;
    int k = 0; //index of cmds
    int flag = 0;
    command* cur_cmd = (command*)malloc(sizeof(command));
    *cmds = (commands**)malloc(sizeof(command*));
    word* wrd;
    for (i = 0; i < cmd->length; i++) {
        if (!(strcmp(cmd->args[i]->data, "|")) {
            if (!k) {
                free(cur_cmd);
                free(*cmds);
                return -1;
            }
            *cmds = (command**)realloc(sizeof(command*) * (k + 1));
            add_last_to_command(cur_cmd);
            is_bg_or_redirection(cur_cmd);
            *cmds[k] = cur_cmd;
            cur_cmd = (command*)malloc(sizeof(command));
            k++;
            if (!flag) {
                *operations = (int*)malloc(sizeof(int));
                j = 0;
            } else {
                *operations = (int*)realloc(sizeof(int) * (j + 1));
            }
            *operations[j] = 0;
            j++;
        } else if (!(strcmp(cmd->args[i]->data, ";"))  {
            if (!k) {
                free(cur_cmd);
                free(*cmds);
                return -1;
            }
            *cmds = (command**)realloc(sizeof(command*) * (k + 1));
            add_last_to_command(cur_cmd);
            is_bg_or_redirection(cur_cmd);
            *cmds[k] = cur_cmd;
            cur_cmd = (command*)malloc(sizeof(command));
            k++;
            if (!flag) {
                *operations = (int*)malloc(sizeof(int));
                j = 0;
            } else {
                *operations = (int*)realloc(sizeof(int) * (j + 1));
            }
            *operations[j] = 1;
            j++;
        } else if (!(strcmp(cmd->args[i]->data, "&&")) {
            if (!k) {
                free(cur_cmd);
                free(*cmds);
                return -1;
            }
            *cmds = (command**)realloc(sizeof(command*) * (k + 1));
            add_last_to_command(cur_cmd);
            is_bg_or_redirection(cur_cmd);
            *cmds[k] = cur_cmd;
            cur_cmd = (command*)malloc(sizeof(command));
            k++;
            if (!flag) {
                *operations = (int*)malloc(sizeof(int));
                j = 0;
            } else {
                *operations = (int*)realloc(sizeof(int) * (j + 1));
            }
            *operations[j] = 2;
            j++;
        } else if (!(strcmp(cmd->args[i]->data, "||")) {
            if (!k) {
                free(cur_cmd);
                free(*cmds);
                return -1;
            }
            *cmds = (command**)realloc(sizeof(command*) * (k + 1));
            add_last_to_command(cur_cmd);
            is_bg_or_redirection(cur_cmd);
            *cmds[k] = cur_cmd;
            cur_cmd = (command*)malloc(sizeof(command));
            k++;
            if (!flag) {
                *operations = (int*)malloc(sizeof(int));
                j = 0;
            } else {
                *operations = (int*)realloc(sizeof(int) * (j + 1));
            }
            *operations[j] = 3;
            j++;
        } else {
            add_argument_to_command(cur_cmd, cmd->args[i]);
        }
    }
    *cmds = (command**)realloc(sizeof(command*) * (k + 1));
    add_last_to_command(cur_cmd);
    is_bg_or_redirection(cur_cmd);
    *cmds[k] = cur_cmd;
    return (k + 1);
}











void print_current_path() {
    char path[MAX_PATH_LENGTH];
    getcwd(path, MAX_PATH_LENGTH);
    printf(">>> %s: ", path);
}

const char** command_st(command* cmd, int remove_last) {
    int n = cmd->length;
    if (remove_last) {
        n--;
    }
    char** args = (char**)malloc((n + 1) * sizeof(char*));
    for (int i = 0; i < n; i++) {
        args[i] = cmd->args[i]->data;
    }
    args[cmd->length] = NULL;
    return (const char**)args;
}

void execute_command_simple(command* cmd) {
    const char** args = command_st(cmd, 0);
    delete_command(cmd);
    execvp(args[0], args);
    printf("ERROR while executing\n");
    exit(1);
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

void execute_command_with_redirection(command *cmd) {
    int redirection = cmd->redirection_state;
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
            mode = "a";
        }
        redirect_stream_to_file(stream, file_name, mode, &fd, &pos);
        execute_command_str(command_str);
        restore_stream(stream, fd, pos);
    }
}

pid_t run_cmd_bg (command* cmd, NODE** bg_pids) {
    int status;
    pid_t pid = fork();
    if (pid == 0) {
        // child process
        #ifdef DEBUG
        char* s;
        command_to_string(cmd, &s);
        printf("RUNNING BG JOB '%s' in process %d\n", s, getpid());
        free(s);
        #endif

        const char** cmd_st = command_st(cmd, 1); 
        execvp(cmd_st[0], (char* const*)cmd_st);
        printf("ERROR while executing\n");
        exit(1);
    } else if (pid < 0) {
        // child failure
        printf("ERROR creating a child\n");
    } else {
        list_add(bg_pids, pid);
    }
    return pid;
}

void bg_wait(NODE** bg_pids, int opt) {
    int flg = 1;
    while (flg) {
        flg = 0;
        NODE* t = *bg_pids;
        while (t) {
            int status;
            waitpid(t->data, &status, opt);
            if (WIFEXITED(status)) {
                #ifdef DEBUG
                printf("BG JOB %d FINISHED\n", t->data);
                #endif
                list_remove(bg_pids, t->data);
                flg = 1;
                break;
            }
            t = t->next;
        }
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

void spawn_proc(int in, int out, struct command* cmd) {
    if ((fork()) == 0) {
        if (in != 0) {
            dup2(in, 0);
            close(in);
        }
        if (out != 1) {
            dup2(out, 1);
            close(out);
        }
        const char** cmd_st = command_st(cmd, 0); 
        execvp(cmd_st[0], (char *const *)cmd_st);
        printf("ERROR while executing\n");
        exit(1);
    }
}

int execute_pipeline(command** pipeline, int n) {
    pid_t pid;
    if ((pid = fork()) == 0) {
        int i;
        int in, fd[2];
        in = 0;
        for (i = 0; i < n - 1; ++i) {
            pipe(fd);
            spawn_proc(in, fd[1], pipeline[i]);
            close(fd[1]);
            in = fd[0];
        }
        if (in != 0) {
            dup2(in, 0);
        }
        command* last = pipeline[n - 1];
        const char** cmd_st = command_st(last, 0); 
        execvp(cmd_st[0], (char *const *)cmd_st);
        printf("ERROR while executing\n");
        exit(1);
    }
    waitpid(pid, NULL, 0);
    return 0;
}


int main() {
    // main shell loop
    int exit = 0;
    NODE* bg_pids = NULL;
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
            command** cmds;
            int* operations;
            int length = split_to_operations(&cmd, &cmds, &operations);
            //int r = split_command_through_pipeline(&cmd, &pipeline, &length);
            if (length < 0) {
                printf("SYNTAX ERROR\n");
                delete_command(&cmd);
                continue;
            }

            /*int r2 = check_pipeline_bg(pipeline, length);
            if (r2) {
                printf("ERROR IN BG SYNTAX\n");
                delete_command(&cmd);
                continue;
            }
            if (length > 1) {
                exit = execute_pipeline(pipeline, length);
            } else {
                if (pipeline[0]->bg) {
                    run_cmd_bg(pipeline[0], &bg_pids);
                } else {
                    exit = execute_command(pipeline[0]);
                }
            }
            delete_pipeline(pipeline, length);
            */
        }        
        delete_command(&cmd);
        bg_wait(&bg_pids, 0);
    }

    return 0;
}

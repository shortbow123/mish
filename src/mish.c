#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>

#include "mish.h"
#include "builtins.h"

/* mish (Mini-shell)
 *  Simplistic Linux/Unix shell that I'm
 *  developing to be my personal shell.
 *  Ryan Danver (Visual-mov) 2020.
 */

int main(int argc, char* argv[]) {
    //signal(SIGINT, SIG_IGN);
    char** args;
    char* cwd;
    char* usr = getenv("USER");
    char* line;

    /* mish shell loop*/
    while(status) {
        cwd = get_dir();
        check_alloc_ptr(cwd);

        printf(USR_COLOR "%s%s : %s%s%s $ ", usr, RESET, DIR_COLOR, cwd, RESET);

        line = read_line();
        args = parse_line(line);

        if(args != NULL && args[0] != NULL) {
            if(in_mish_cmds(args[0]))
                exec_mish_cmd(args[0], args);
            else
                exec_program(args);
        }

        free_args(args);
        free(line);
        free(cwd);
    }
    return 0;
}

/* Splits line into args */
char** parse_line(char* line) {
    int args_buf = ARGS_BUF;
    int args_index = 0, i = 0, last_delim = 0;

    char** args = (char**) malloc(args_buf * sizeof(char*));
    check_alloc_ptr(args);

    while(i < strlen(line)) {
        if(i == args_buf-1) {
            args_buf += ARGS_BUF;
            if(!(args = (char**) realloc(args, args_buf * sizeof(char*)))) {
                MISH_ERR("memory allocation error");
                status = 0;
            }
        }
        
        if (line[i] == ' ' || line[i] == '\n') {
            char* token = sub_string(line, last_delim, i-1);
            args[args_index] = token;
            args_index++;
            last_delim = i+1;
        }
        i++;
    }
    args[args_index] = NULL;
    args_len = args_index;
    return args;
}

/* Frees all alloced space for arguments. */
void free_args(char** args) {
    for(int i = 0; i < args_len; i++) {
        free(args[i]);
    }
    free(args);
}

char* sub_string(char* str, int l, int r) {
    char* substring = (char*) malloc(r - l + 2);
    for(int i = l; i <= r; i++)
        substring[i-l] = str[i];
    substring[r-l+1] = '\0';
    return substring;
}

/* Executes builtin mish commands */
void exec_mish_cmd(char* cmd, char** args) {
    if(strcmp(cmd, "help") == 0)
        mish_help();
    else if(strcmp(cmd, "exit") == 0)
        status = 0;
    else if(strcmp(cmd, "cd") == 0)
        mish_cd(args);
    else if(strcmp(cmd, "~") == 0)
        mish_home();
    else if(strcmp(cmd, "/") == 0)
        mish_root();
    else if(strcmp(cmd, "..") == 0)
        mish_cd_up();
}

/* Create child process and execute program with arguments. */
void exec_program(char** args) {
    pid_t pid = fork();
    int stat;
    if(pid == 0) {
        // Don't suppress signal interrupt for child process.
        signal(SIGINT, SIG_DFL);
        // If execution was unsuccessful, print the cause and terminate the child process.
        if(execvp(args[0], args) == -1) {
            char* err;
            switch(errno) {
                default: err = "execution error"; break;
                case ENOENT: err = "command not found"; break;
                case EACCES: err = "permission denied"; break;
            }
            printf("%s: %s\n", args[0], err);
            status = 0;
        }
    } else if(pid > 0) {
        waitpid(pid, &stat, WUNTRACED);
        //signal(SIGINT, SIG_IGN);
    }
}

/* Reads line from stdin, reallocing as needed */
char* read_line() {
    char* line = NULL;
    size_t n;
    getline(&line, &n, stdin);
    return line;
}

/* Gets working directory for shell */
char* get_dir() {
    int cwd_buf = STR_BUF;
    char* cwd = (char*) malloc(cwd_buf);

    while(getcwd(cwd, cwd_buf) == NULL && errno == ERANGE) {
        cwd_buf += STR_BUF;
        cwd = realloc(cwd, cwd_buf);
    }
    
    return cwd;
}

/* Checks if memory hadn't been successfully allocated, and prints error */
void check_alloc_ptr(void* p) {
    if(!p) {
        MISH_ERR("memory allocation error");
        status = 0;
    }
}

// void enable_raw(int enable) {
//     if(enable) {
//         tcgetattr(STDIN_FILENO, &cooked);
//         struct termios raw;
//         tcgetattr(STDIN_FILENO, &raw);
//         raw.c_lflag &= ~(ECHO);
//         tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
//     } else {
//         tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked);
//     }
// }
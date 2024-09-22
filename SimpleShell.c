#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h> 
#include <ctype.h> 

#define MAX_COMMANDS 10
#define MAX_ARGS 100

typedef struct {
    char* cmd;
    int pid;
    time_t initial_time;
    double duration;
} history;

int cntr = 0;
history hst[250];

void histry() {
    for (int i = 0; i < cntr; i++) {
        printf("Given command-%d was: %s\n", i + 1, hst[i].cmd);
        printf("Starting time for process: %s", ctime(&hst[i].initial_time));
        printf("Process pid is: %d\n", hst[i].pid);
        printf("Process duration is: %f seconds\n", hst[i].duration);
        printf("\n");
    }
}

int create_and_run(char *command) {
    clock_t start_time = clock();
    int child_st = fork();
    time_t initial_time;
    time(&initial_time);
    
    if (child_st < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    
    if (child_st == 0) {
        char *argv[1024];
        char *temp = strtok(command, " ");
        if (temp == NULL) {
            fprintf(stderr, "Error in tokenising string\n");
            exit(EXIT_FAILURE);
        }
        int pos = 0;
        while (temp != NULL) {
            argv[pos++] = temp;
            temp = strtok(NULL, " ");
        }
        argv[pos] = NULL;

        if (execvp(argv[0], argv) == -1) {
            perror("Exec error");
            exit(EXIT_FAILURE);
        }
    } else {
        int status;
        if (waitpid(child_st, &status, 0) == -1) {
            perror("Wait failed");
        }
        clock_t end_time = clock();
        hst[cntr].cmd = malloc(strlen(command) + 1);
        if (hst[cntr].cmd == NULL) {
            perror("Error in allocating size");
            exit(EXIT_FAILURE);
        }
        strcpy(hst[cntr].cmd, command);
        hst[cntr].pid = child_st;
        hst[cntr].initial_time = initial_time;
        hst[cntr].duration = ((double)(end_time - start_time) / CLOCKS_PER_SEC);
        cntr++;
    }
    return 1;
}

int launch(char *command) {
    if (strcmp(command, "history") == 0) { 
        clock_t start_time = clock();
        time_t initial_time;
        time(&initial_time);
        hst[cntr].cmd = malloc(strlen(command) + 1);
        if (hst[cntr].cmd == NULL) {
            perror("Error in allocating size");
            exit(EXIT_FAILURE);
        }
        strcpy(hst[cntr].cmd, command);
        hst[cntr].pid = getpid();
        hst[cntr].initial_time = initial_time;
        clock_t end_time = clock();
        hst[cntr].duration = ((double)(end_time - start_time) / CLOCKS_PER_SEC);
        cntr++;
        histry();
        
        return 1;
    } else if (strcmp(command, "exit") == 0) { 
        histry();
        return 0;
    }
    return create_and_run(command);
}

void my_handler(int signum) {
    printf("Exited Successfully \n");
    histry();
    exit(0);
}

void slice(char *s, char *t, int st, int en) {
    if (st <= en) {
        int o = 0;
        for (int i = st; i <= en; ++i) {
            t[o++] = s[i];
        }
        t[o] = '\0';
    }
}

void pipeC(char *i) {
    char *cm = malloc(strlen(i) + 1);
    if (cm == NULL) {
        perror("Error in allocating space");
        exit(EXIT_FAILURE);
    }
    strcpy(cm, i);
    
    char *commands[MAX_COMMANDS];
    int num_commands = 0;
    
    char *cmd = strtok(cm, "|");
    while (cmd != NULL && num_commands < MAX_COMMANDS) {
        commands[num_commands] = malloc(strlen(cmd) + 1);
        if (commands[num_commands] == NULL) {
            perror("Error in allocating command space");
            exit(EXIT_FAILURE);
        }
        strcpy(commands[num_commands], cmd);
        cmd = strtok(NULL, "|");
        num_commands++;
    }

    int pipe_fd[2 * (num_commands - 1)];

    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipe_fd + i * 2) == -1) {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid;
    for (int i = 0; i < num_commands; i++) {
        pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            if (i > 0) { 
                dup2(pipe_fd[(i - 1) * 2], 0); 
            }
            if (i < num_commands - 1) { 
                dup2(pipe_fd[i * 2 + 1], 1); 
            }

            for (int j = 0; j < 2 * (num_commands - 1); j++) {
                close(pipe_fd[j]);
            }

            char *args[MAX_ARGS];
            char *cmd = commands[i];
            char *arg = strtok(cmd, " ");
            int arg_count = 0;

            while (arg != NULL) {
                args[arg_count++] = arg;
                arg = strtok(NULL, " ");
            }
            args[arg_count] = NULL; 

            if (execvp(args[0], args) == -1) {
                perror("Execvp error");
                exit(EXIT_FAILURE);
            }
        }
    }

    for (int i = 0; i < 2 * (num_commands - 1); i++) {
        close(pipe_fd[i]);
    }

    for (int i = 0; i < num_commands; i++) {
        wait(NULL);
    }

    clock_t end_time = clock();
    hst[cntr].cmd = malloc(strlen(i) + 1);
    if (hst[cntr].cmd == NULL) {
        perror("Error in allocating size");
        exit(EXIT_FAILURE);
    }
    strcpy(hst[cntr].cmd, i);
    hst[cntr].pid = pid;
    hst[cntr].initial_time = time(NULL);
    hst[cntr].duration = ((double)(end_time - start_time) / CLOCKS_PER_SEC);
    cntr++; 

    for (int j = 0; j < num_commands; j++) {
        free(commands[j]);
    }
    free(cm);
}

int pipe_call(char i[]) {
    i[strcspn(i, "\n")] = 0;
    pipeC(i);
    return 1;
}

int main() {
    int status;
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig)); 
    sig.sa_handler = my_handler;
    if (sigaction(SIGINT, &sig, NULL) == -1) {
        perror("sigaction error");
        exit(EXIT_FAILURE);
    }
    
    do {
        printf("gaur-garg-iiitd.ac: ");
        char command[500];
        if (fgets(command, sizeof(command), stdin) == NULL) { 
            perror("fgets error");
            exit(EXIT_FAILURE);
        }

        int bool = 0;
        int len = strlen(command);
        for (int i = 0; i < len; i++) {
            if (command[i] == '|') {
                status = pipe_call(command);
                bool = 1;
                break;
            }
        }
        
        if (bool == 0) {
            if (len > 0 && command[len - 1] == '\n') {
                command[len - 1] = '\0';
            }
            status = launch(command);
        }
        
    } while (status);
    
    printf("Completed shell process\n");
    return 0;
}

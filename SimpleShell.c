#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h> 
#include <ctype.h> 
#include <readline/readline.h>
#include <readline/history.h>


#define MAX_COMMANDS 100
#define MAX_ARGS 100

typedef struct {
    char* cmd;
    int pid;
    time_t initial_time;
    int backgrnd;
    double duration;
} history;

int cntr = 0;
history hst[250];

void histry() {
    for (int i = 0; i < cntr; i++) {
        printf("Given command-%d was: %s\n", i + 1, hst[i].cmd);
        printf("Starting time for process: %s", ctime(&hst[i].initial_time));
        if(hst[i].backgrnd==1){
            printf("Background process\n");
        }
        printf("Process pid is: %d\n", hst[i].pid);
        printf("Process duration is: %f seconds\n", hst[i].duration);
        printf("\n");
    }
}
void remove_character(char *str, char char_to_remove) {
    int i, j = 0;
    int len = strlen(str);
    char result[len + 1]; 

    for (i = 0; i < len; i++) {
        if (str[i] != char_to_remove) {
            result[j++] = str[i];
        }
    }
    result[j] = '\0'; 
    strcpy(str, result);
}
int create_and_run(char *command) {
    clock_t start_time = clock();
    int child_st = fork();
    time_t initial_time;
    time(&initial_time);
    
    if (child_st < 0) {
        perror("Fork failed");
        exit(0);
    }
    int background=0;
    if (strchr(command, '&')) {
        background = 1;
        remove_character(command, '&');
    }
    
    if (child_st == 0) {
        char *argv[1024];
        char *temp = strtok(command, " ");
        if (temp == NULL) {
            printf("Error in tokenising string\n");
            exit(EXIT_FAILURE);
        }
        int pos = 0;
        while (temp != NULL) {
            argv[pos++] = temp;
            temp = strtok(NULL, " ");
        }
        argv[pos] = NULL;
        if (execvp(argv[0], argv) == -1) {
            printf("Exec error\n");
            exit(EXIT_FAILURE);
        }
        exit(1);
    } else {
        int status;
        if(!background){
          if (waitpid(child_st, &status, 0) == -1) {
              printf("Wait failed\n");
              exit(EXIT_FAILURE);
            }
        }
        clock_t end_time = clock();
        hst[cntr].cmd = malloc(strlen(command) + 1);
        if (hst[cntr].cmd == NULL) {
            printf("Error in allocating size\n");
            exit(EXIT_FAILURE);
        }
        if(background){
            hst[cntr].backgrnd=1;
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
    if (strncmp(command, "cd", 2) == 0) {
        clock_t start_time = clock();
        time_t initial_time;
        time(&initial_time);
        char *dir = strtok(command + 3, " ");
        if (dir == NULL) {
            dir = getenv("HOME");
        }
        if (chdir(dir) != 0) {
            printf("cd failed\n");
            exit(EXIT_FAILURE);
        }
        hst[cntr].cmd = malloc(strlen(command) + 1);
        if (hst[cntr].cmd == NULL) {
            printf("Error in allocating size\n");
            exit(EXIT_FAILURE);
        }
        strcpy(hst[cntr].cmd, command);
        hst[cntr].pid = getpid();
        hst[cntr].initial_time = initial_time;
        clock_t end_time = clock();
        hst[cntr].duration = ((double)(end_time - start_time) / CLOCKS_PER_SEC);
        cntr++;
        return 1;
    } 
    if (strcmp(command, "history") == 0) { 
        clock_t start_time = clock();
        time_t initial_time;
        time(&initial_time);
        hst[cntr].cmd = malloc(strlen(command) + 1);
        if (hst[cntr].cmd == NULL) {
            printf("Error in allocating size\n");
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
    if(signum==SIGINT){
       printf("Exited Successfully \n");
       histry();
       exit(0);
    }
    if(signum==SIGCHLD){
        while((waitpid(-1, NULL, WNOHANG) > 0));
    }
}

void slice(char *s, char *t, int st, int en) {
    if (st <= en) {
        int o = 0;
        for (int i = st; i <= en; ++i) {
            t[o] = s[i];
            o++;
        }
        t[o] = '\0';
    }
}

void pipeC(char *i) {
    int background=0;
    if (strchr(i, '&')) {
        background = 1;
    
       remove_character(i, '&');
    }
    char *cmdd = malloc(strlen(i)+1);
    strcpy(cmdd,i);
    char *cm = strtok(i, "|");
    if (cm == NULL) {
        printf("Error in allocating space");
        exit(EXIT_FAILURE);
    }
    time_t start_time=clock();
    
    char *commands[MAX_COMMANDS];
    int num_commands = 0;
    
    //char *cmd = strtok(cm, "|");
    while (cm != NULL && num_commands < MAX_COMMANDS) {
        commands[num_commands] = (char*)malloc(strlen(cm) + 1);
        if (commands[num_commands] == NULL) {
            printf("Error in allocating command space");
            exit(EXIT_FAILURE);
        }
        slice(cm,commands[num_commands],0,strlen(cm)-1);
        cm = strtok(NULL, "|");
        num_commands++;
    }

    int pipe_fd[2 * (num_commands - 1)];

    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipe_fd + i * 2) == -1) {
            printf("Pipe creation failed\n");
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
            char *cmd1 = commands[i];
            cm= strtok(cmd1, " ");
            int arg_count = 0;

            while (cm != NULL) {
                args[arg_count++] = cm;
                cm= strtok(NULL, " ");
            }
            args[arg_count] = NULL; 
            if (execvp(args[0], args) == -1) {
                printf("Execvp error\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    for (int i = 0; i < 2 * (num_commands - 1); i++) {
        close(pipe_fd[i]);
    }
    int status;
    if(!background){
     for (int i = 0; i < num_commands-1; i++) {
          if (wait(NULL) == -1) {
              printf("Wait failed\n");
              exit(EXIT_FAILURE);
            }
     }
    }

    clock_t end_time = clock();
    hst[cntr].cmd = malloc(strlen(cmdd) + 1);
    if (hst[cntr].cmd == NULL) {
        printf("Error in allocating size\n");
        exit(EXIT_FAILURE);
    }
    strcpy(hst[cntr].cmd, cmdd);
    hst[cntr].pid = pid;
    if(background){
        hst[cntr].backgrnd=1;
    }
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
        printf("sigaction error\n");
        exit(EXIT_FAILURE);
    }
    if(sigaction(SIGCHLD,&sig,NULL)== -1){
        printf("sigaction error\n");
        exit(EXIT_FAILURE);
    }
    do{
        char* command;
        command=malloc(500);
        if(command==NULL){
            printf("Allocation not done\n");
            exit(EXIT_FAILURE);
        }
        command=readline("gaur-garg-iiitd.ac:");
        if(command[0] == '\0'){
            continue;
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

        if(command){
            free(command);
        }
        
    }while (status);
    
    printf("Completed shell process\n");
    return 0;
}

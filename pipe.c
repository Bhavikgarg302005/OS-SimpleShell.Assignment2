#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_COMMANDS 10
#define MAX_ARGS 100

char* slice(char *s,char*t,int st,int en)
{
    
    if(st<=en)
    {
        int o=0;
        for(int i=st;i<=en;++i)
        {
            t[o]=s[i];
            o++;
        }
        t[o]='\0';
        
    }
    

}

void pipeC(char *i) {
    
    char *cm=strtok(i, "|");
    int num_commands = 0;
    char *commands[MAX_COMMANDS];
    // spliting the input string into individual commands
    while (cm != NULL && num_commands < MAX_COMMANDS) {
        
        commands[num_commands] = (char*) malloc(strlen(cm) + 1);  // +1 for null terminator
        slice(cm, commands[num_commands], 0, strlen(cm)-1);  
        cm= strtok(NULL, "|");
        num_commands++;
        
        
        
    }
    

    int pipe_fd[2 * (num_commands - 1)];  // Create pipe file descriptors

    // Create pipes
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipe_fd + i * 2) == -1) {
            printf("SOme error while making pipes");
            exit(0);
        }
    }

    // Fork and execute each command
    for (int i = 0; i < num_commands; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            printf("Some error in making a process");
            exit(0);
        } else if (pid == 0) {
            // Child process
            if (i > 0) { // If not the first command, get input from the previous pipe
                dup2(pipe_fd[(i-1)*2], 0); // Read end of the previous pipe
            }

            if (i < num_commands - 1) { // If not the last command, output to the next pipe
                dup2(pipe_fd[i*2+1],1); // Write end of the current pipe
            }

            // Close all pipe fds in the child
            for (int j = 0; j < 2 * (num_commands - 1); j++) {
                close(pipe_fd[j]);
            }

            // Tokenize the command into arguments
            char *args[MAX_ARGS];
            char *cmd = commands[i];
            cm= strtok(cmd, " ");
            int arg_count = 0;

            while (cm != NULL) {
                args[arg_count++] = cm;
                cm= strtok(NULL, " ");
            }
            args[arg_count] = NULL; // Null-terminate the arguments

            // Execute the command
            if(execvp(args[0], args)==-1)
            {
                printf("Some error in execvp\n");
                exit(0);
            }
            
            
        }
    }

    // Parent process closes all pipe ends
    for (int i = 0; i < 2 * (num_commands - 1); i++) {
        close(pipe_fd[i]);
    }

    // Parent waits for all children
    for (int i = 0; i < num_commands; i++) {
        wait(NULL);
    }
}

int main() {
    char i[1024];
    printf("Enter command: ");
    if (fgets(i, sizeof(i), stdin) == NULL) {
        printf("Error in fgets");
        return 1;
    }
    i[strcspn(i, "\n")] = 0;
    printf("%s\n",i);
    pipeC(i);
    return 0;
}

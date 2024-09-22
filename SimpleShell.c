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


typedef struct{
  char* cmd;
  int pid;
  time_t initial_time;
  double duration;
} history;
int cntr=0;
history hst[250];
void histry(){
  for(int i=0;i<cntr;i++){
    printf("Given command-%d was:%s\n",i+1,hst[i].cmd);
    printf("Starting time for process:%s",ctime(&hst[i].initial_time));
    printf("process pid is:%d\n",hst[i].pid);
    printf("process duration is:%f seconds\n",hst[i].duration);
    printf("\n");
  }
}
int create_and_run(char *command){
    clock_t start_time = clock();
    int child_st=fork();
    time_t initial_time;
    time(&initial_time);
    if(child_st<0){
      printf("error in fork command");
      exit(0);
    }
    if(child_st==0){
      char *argv[1024];
      char* temp=strtok(command," ");
      if(temp==NULL){
        printf("Error in tokenising string\n");
        exit(0);
      }
      int pos=0; 
      while(temp!=NULL){
        argv[pos]=temp;
        temp=strtok(NULL," ");
        pos++;
      }
      argv[pos]=NULL;
      
       if(execvp(argv[0], argv) == -1){
          printf("Exec error\n");
          exit(0);
        }
    } 
      else{
        waitpid(child_st,NULL, 0);
        clock_t end_time = clock();
        hst[cntr].cmd =malloc(strlen(command) + 1);
        if(hst[cntr].cmd==NULL){
          printf("Error in allocating size\n");
          exit(0);
        }
        strcpy(hst[cntr].cmd,command);
        hst[cntr].pid=child_st;
        hst[cntr].initial_time=initial_time;
        hst[cntr].duration=((double)(end_time-start_time)/ CLOCKS_PER_SEC);
        cntr++;        
      }
      return 1;
}

int launch(char* command){
  if (strcmp(command, "history") == 0) { 
        clock_t start_time = clock();
        time_t initial_time;
        time(&initial_time);
        hst[cntr].cmd =malloc(strlen(command) + 1);
        if(hst[cntr].cmd==NULL){
          printf("Error in allocating size\n");
          exit(0);
        }
        strcpy(hst[cntr].cmd,command);
        hst[cntr].pid=getpid();
        hst[cntr].initial_time=initial_time;
        clock_t end_time=clock();
        hst[cntr].duration=((double)(end_time-start_time)/ CLOCKS_PER_SEC);
        cntr++; 
        histry();
        
        return 1;
    }
    else if (strcmp(command, "exit") == 0) { 
        histry();
        return 0;
    }
  int st;
  st=create_and_run(command);
  return st;
}
void my_handler(int signum) {
    printf("Exited Succesfully \n");
    histry();
    exit(0);
}
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
    char *t;
    t=malloc(strlen(i)+1);
    t=i;
    for(int j=0;j<strlen(i);j++){
      t[j]=i[j];
    }
    char *cm=malloc(strlen(i)+1);
    strcpy(cm,i);
    cm=strtok(cm, "|");
    int num_commands = 0;
    char *commands[MAX_COMMANDS];
    clock_t start_time = clock();
    time_t initial_time;
    time(&initial_time);
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
    pid_t pid;
    // Fork and execute each command
    for (int i = 0; i < num_commands; i++) {
        pid = fork();

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
    clock_t end_time = clock();
    hst[cntr].cmd =malloc(strlen(t) + 1);
    if(hst[cntr].cmd==NULL){
      printf("Error in allocating size\n");
      exit(0);
    }
    strcpy(hst[cntr].cmd,t);
    hst[cntr].pid=pid;
    hst[cntr].initial_time=initial_time;
    hst[cntr].duration=((double)(end_time-start_time)/ CLOCKS_PER_SEC);
    cntr++; 

}

int pipe_call(char i[]){
    i[strcspn(i, "\n")] = 0;
    pipeC(i);
    return 1;
}
int main(){
  int status;
  struct sigaction sig;
  memset(&sig, 0, sizeof(sig)); 
  sig.sa_handler = my_handler;
  sigaction(SIGINT, &sig, NULL);
  if (sigaction(SIGINT, &sig, NULL) == -1) {
    printf("sigaction error");
    exit(1);
  }
  do{
    printf("gaur-garg-iiitd.ac:");
    char command[500];
    if(fgets(command, sizeof(command), stdin) == NULL){ 
          // The fgets function reads a line of input from stdin (standard input) and stores it in the input array. 
        printf("fgets error");
        exit(1);
    }
    int bool=0;
    int len = strlen(command);
    for(int i=0;i<len;i++){
      if(command[i]=='|'){
        printf("going");
        status=pipe_call(command);
        bool=1;
        break;
      }
    }
    if(bool==0){
     if (len > 0 && command[len - 1] == '\n') {
      command[len - 1] = '\0';
     }
     //checking that piped commands are there or not:
     status=launch(command);
    }
        
    }while(status);
    
    printf("completed shell process\n");
  }


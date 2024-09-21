#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h> 
#include <ctype.h> 
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
    printf("Starting time for process:%s\n",ctime(&hst[i].initial_time));
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
    int len = strlen(command);
    if (len > 0 && command[len - 1] == '\n') {
      command[len - 1] = '\0';
    }
    //checking that piped commands are there or not:
    status=launch(command);
        
    }while(status);
    
    printf("completed shell process\n");
  }

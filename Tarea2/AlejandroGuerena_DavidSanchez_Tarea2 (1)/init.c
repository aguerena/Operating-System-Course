#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>

//#define DEBUG 

// Number of child process to run
#define PROGRAMS 6


void createProcess(){

    int i;
    pid_t pid;	
    char *execArgs[] = {"xterm", "-e", "./getty", NULL};

    #ifdef DEBUG
    printf("CREATING PROCESS\n");
    #endif
    pid = fork();

    if(pid == (-1)){
        printf("\n Fork failed, quitting\n");
        exit (1);
    }
    else if (pid > 0){
        // Parent Process
        #ifdef DEBUG
        printf("Parent Process PID %d pid %d \n", getpid(), pid);
        #endif
    }
    else {
        // Child Process
        #ifdef DEBUG
        printf("Child Process Child PID %d pid %d \n", getpid(), pid);
        #endif
        execvp("/usr/bin/xterm", execArgs);
    }            
		
}


int main(){

    int i, status;
    pid_t w;

    // Execute getty Programs    
    for(i=0;i<PROGRAMS;i++){
	createProcess();
    }
      
    while(1){
        
	// Wait for any child
        w = waitpid(WAIT_ANY, &status, 0);
           
        if (w == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
	
	// Kill signal is detected. Terminate Program        
        if(WIFSIGNALED(status)){
            if(WTERMSIG(status) == 9){
                kill(0, SIGKILL);
                perror("waitpid");
                break;
            }
        }

	// Exit is detected. Create another Process
        if (WIFEXITED(status)) {  
            #ifdef DEBUG
            printf("exited, Child Process Number %d Exit status= %d \n", w, WEXITSTATUS(status));
            #endif
	    createProcess();
        }    
   
    }
				  
    return 1;
}





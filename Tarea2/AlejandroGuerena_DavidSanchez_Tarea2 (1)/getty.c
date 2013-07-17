#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

//#define DEBUG

void loginUser();
void createProcess();

int main(){

    int status;	
    pid_t w;
    
    // Show login prompt
    loginUser();

    while(1){
        
        // Wait for any child
        w = waitpid(WAIT_ANY, &status, 0);
           
        if (w == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
	
        // Exit is detected. Check exit status	
        if (WIFEXITED(status)) {
            #ifdef DEBUG  
            printf("exited, Child Process Number %d status=%d\n", w, WEXITSTATUS(status));
	    #endif
            // Check for Shutdown Signal
            if(WEXITSTATUS(status) == 3){
                // Send kill signal to parent
                #ifdef DEBUG
                printf("getppid: %d \n", getppid());
                #endif
                printf("EXIT!! \n");
		kill(getppid(), 9);
                break;
            }
            // Another kind of Exit - login prompt is shown again
            else
                loginUser();
	        
        }          
    }
    return 1;
}

void loginUser(){

    // Wait for Correct Login
    while ( gettingUser() == 0)
        printf("Usuario y/o Password Incorrectos\n");
	
    printf("Usuario y Password Correctos\n");
    createProcess();
   
}

void createProcess(){

    int i;
    pid_t pid;	
    const char* command = "./sh";

    pid = fork();

    if(pid == (-1)){
        printf("\n Fork failed, quitting\n");
        exit (1);
    }
    else if (pid > 0){
        //Parent Process
    }
    else {
        //Child Process
        execv(command, NULL); 
    }            	

}



int gettingUser(){

    FILE *fp;
    char usuario  [20];
    char password [20]; 
    char fileusr  [20];
    char filepwd  [20];
    int status; // 1=Correct Login, 0=Incorrect Login

    printf("Usuario: ");
    gets(usuario);
    printf("Password: ");
    gets(password);

    status = 0;

    // Open file for read
    fp = fopen("shadow.txt", "r");
    if (fp == NULL){
        printf("File could not be open\n");
	exit(0);
    }

    // Split string, delimeter ":"
    // Wait for two strings, User and Password
    while( (fscanf(fp, "%[^':']:%s\n", fileusr, filepwd)) == 2 ){
        // Check for User
        if(strcmp(fileusr,usuario) == 0){
            // Check user password
            if(strcmp(filepwd,password) == 0){
                // Correct login
	        status = 1;
	        break;
	    }	
	}		
    }
    // Close file
    fclose(fp);
    return status;	
}



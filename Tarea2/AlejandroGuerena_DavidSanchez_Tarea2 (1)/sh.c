#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>


int main(){

    FILE *fp;
    char shcommand [256];
    int bcount, bckgprocess, status;
    pid_t pid;
    char *result = NULL;
    char *execPrgs[256];
    char *execPrgsfirst;
    char env_var [256];
    char val_env_var [256];
    char *ptval_env_var;

    printf("Welcome...\n");

    while(1){
              
        // Getting command  
        printf("> ");
        gets(shcommand);
        #ifdef DEBUG
        printf("Comando insertado : %s \n", shcommand );	
        #endif

        // Check for exit command. Return Exit status 0
        if(strcasecmp(shcommand, "exit") == 0){
            printf("Bye....\n");
            wait(&status);
	    exit(0);        
	}

        // Check for shutdown command. Return Exit status 3
        if(strcasecmp(shcommand, "shutdown") == 0){
            printf("Terminate All....\n");
            wait(&status);
	    exit(3);        
	}

        bcount = 0;
        bckgprocess = 0;       

        // Split Command received with delimiter " "
        result = strtok(shcommand, " ");
        while( result != NULL) {
            #ifdef DEBUG
            printf( "result is \"%s\"\n", result );
            #endif
            execPrgs[bcount++] = result;
            result = strtok( NULL, " " );
        }
        execPrgs[bcount] = NULL;        

        // Check if one more command characters were received
        if(bcount > 0){

            // Check for background procces
            if(strcasecmp(execPrgs[bcount-1], "&") == 0)
                bckgprocess = 1;

            pid = fork();
            if(pid == (-1)){
                printf("\n Fork failed, quitting\n");
                exit (1);
            }
            else if (pid > 0){
                // Parent Process. Wait for child process, except for background process
                if(bckgprocess == 0)
                    wait(&status);
            }
            else {
                // Child Processes
 
                // ---- Check $ Commands -----
                execPrgsfirst = execPrgs[0];
                if(execPrgsfirst[0] == '$'){
                   
                    // Open Environment Variables file for read
                    fp = fopen("env_vars.txt", "r");
                    if (fp == NULL){
                        printf("File could not be open\n");
	                exit(1);
                    }

                    // Read Environment Variables
                    while( (fscanf(fp, "%[^'=']=%s\n", env_var, val_env_var)) == 2 ){
                        #ifdef DEBUG
                        printf("Valor execPrgs[0]: %s  env_var: %s val_env_var: %s \n", execPrgs[0], env_var, val_env_var);
                        #endif
                        // Compare inserted $COMMAND with existing environment variables 
                        if(strcmp(env_var,execPrgs[0]) == 0){
                            ptval_env_var = val_env_var;
                            // Execute and check if environment variable has a valid command                       
                            if( execlp( ptval_env_var, ptval_env_var, (char*)0 ) == -1){
                                if(errno == ENOENT)
                                    printf("%s: command not found\n", execPrgs[0]);
                                // Close Environment Variables File
                                fclose(fp);
                                // Exit Process
                                exit(1);
                            }
                        }
                    } 
                    // Close Environment Variables file
                    fclose(fp);
                    // Exit Process 
                    exit(0); 
  
                }
                // ---- Check Export Command -----
                else if(strcasecmp(execPrgs[0], "export") == 0){
                    
                    // Check for correct export command format
                    if((execPrgs[1] != NULL) && (bcount < 3) && (sscanf(execPrgs[1], "%[^'=']=%s", env_var, val_env_var) == 2) ){
                        #ifdef DEBUG
                        printf("Value for Export: %s \n",  execPrgs[1]);
                        #endif

                        // Open Environment Variables file for write append
                        fp = fopen("env_vars.txt", "a+");
                        if (fp == NULL){
                            printf("File could not be open\n");
	                    exit(1);
                        }
                        // Write Environment Variable and value on Environment Variable file
                        fprintf(fp, "$%s \n", execPrgs[1]);
                        // Close Environment Variables file
		        fclose(fp);
                    }
                    else{
                        printf("ERROR: Bad command export \n");
                    }
                    // Exit Process
                    exit(0);
                  
		}
                // ---- Check Echo Command -----
                else if(strcasecmp(execPrgs[0], "echo") == 0){

                    // Check correct Echo format command
                    if((execPrgs[1] != NULL) && (bcount < 3)){
                        execPrgsfirst = execPrgs[1];
                        // Check for Environment Variable
                        if(execPrgsfirst[0] == '$'){

                            // Open Environment Variables file for read
                            fp = fopen("env_vars.txt", "r");
                            if (fp == NULL){
                                printf("File could not be open\n");
	                        exit(1);
                            }
                            
                            // Split environment variables with delimiter '=' and check if they equal the echo value    
                            while( (fscanf(fp, "%[^'=']=%s\n", env_var, val_env_var)) == 2 ){
                                #ifdef DEBUG
                                printf("Valor execPrgs[1]: %s  env_var: %s val_env_var: %s \n", execPrgs[1], env_var, val_env_var);
                                #endif
                                // Print Environment variable value if equals the echo value
                                if(strcmp(env_var,execPrgs[1]) == 0){
                                    printf("%s\n", val_env_var);
                                    // Close Environment Variables File
                                    fclose(fp);
                                    // Exit Process
                                    exit(0);
                                } 
                            }
                            // Close Environment Variables File
                            fclose(fp);
                        }
                        // Environment Variable does not exist or is not defined, just print the received echo value
                        printf("%s\n", execPrgs[1]);
                        // Exit Process 
                        exit(0);      
                    }
                    else{
                        printf("ERROR: Bad command echo \n");
                        // Exit Process 
                        exit(1);
                    }    
                
                }
                // ---- Other Commands -----
                // Execute and check if environment variable has a valid command
                else if( execvp(execPrgs[0], execPrgs) == -1) { 
                    if(errno == ENOENT)
                        printf("%s: command not found\n", execPrgs[0]);
                    exit(1);
                }                                        

            }            
        
        }
	
    }	  

    return 1;
}


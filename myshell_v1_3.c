#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>

#define MAXCMDSIZE 100
#define MAXPARAMS 100
#define PATHSIZE 200
#define MAXLEN 500
#define TRUE 1
#define PIPESIG "|"
#define ARRAYLIMIT 255

void clean_up_child_process( int signal_number );

/**@<myshell.c>::
 INPUT: split_command(source, dest)
 DESCRIPTION: Tokenizes the stream given by the user to arg_list*/
int split_command( char *cmd, char **arg_list );

int cmd_execute( char *program, char **arg_list );
void prompt( char *username, char *hostname, char *new_cwd );
char *abbreviate(char *input);

/**@<myshell.c>::
 *  INPUT: piped ( input stream given by the user )
 *  DESCRIPTION: Explode the user input stream in the '|' symbol, 
 generating a list of commands to be executed in pipeline mode
 *  OUTPUT: It fills the array with the input stream broken 
 into commands at PIPESIG*/
void piped ( char *cmd );

sig_atomic_t child_exit_status;
char *array[ARRAYLIMIT]; // list of commands. Stack using FIFO
int ap = 0; // array pointer
int pipe_fd[2]; //pipeline file descriptor

int main( int argc, char* argv[] ){
    
    //char* stream[MAXLEN]; // input stream given by the user
    char cmd[MAXCMDSIZE+1]; // serie of 'cmd' piped
    
    /**@<myshell.c>
     tokenized string*/
    char* cmd_params[MAXPARAMS+1];
    char hostname[128];
    char* cwd; 
    char* new_cwd;
    char pathbuff[PATHSIZE + 1];
  
    struct sigaction sigchld_action;
    memset ( &sigchld_action, 0, sizeof (sigchld_action) );
    sigchld_action.sa_handler = &clean_up_child_process;
    
    while(TRUE){
        
        int cmdindex = 0;
        int i = 0;
        while(i < 10){
            array[i] = "";
            i++;
        }
    
        char *username = getenv ( "USER" ); // get user
        gethostname ( hostname, sizeof(hostname) ); // get host
        cwd = getcwd ( pathbuff, PATHSIZE + 1 ); // get directory
           
        signal ( SIGTSTP, SIG_IGN ); // prevent Ctrl+Z from happening
        signal ( SIGINT, SIG_IGN ); // prevent Ctrl+C from happening
        sigaction ( SIGCHLD, &sigchld_action, NULL ); // quit on Ctrl+D
        
        new_cwd = abbreviate ( cwd );

        prompt( username, hostname, new_cwd );
        
        if ( fgets( cmd, sizeof(cmd), stdin ) == NULL ){
            // enters here when user press Ctrl+D
            fprintf ( stdout, "\n" );
            exit(0);
        } else {
            
            if(cmd[strlen(cmd)-1] == '\n') {
                cmd[strlen(cmd)-1] = '\0';
            }
            
            if( strlen (cmd) != 0 ){
            
                piped( cmd );
            
                
                while (cmdindex < ap) {
                    // error? ch(table* - > cmd)
                    if (strcmp(array[cmdindex], "") != 0 ){
                        
                        split_command(array[cmdindex], cmd_params);
                        
                        // exits on 'exit'
                        if(strcmp(cmd_params[0],"exit") == 0) exit(0);
                        
                        //pipe ( pipe_fd );
                        
                        // execute command otherwise            
                        cmd_execute ( cmd_params[0], cmd_params );   
                    }
                    cmdindex++;
                }
            }
        }
    }
    exit(1);
}

void clean_up_child_process( int signal_number ){
    /* Limpa o processo filho. */
    int status;
    wait ( &status );
    /* Guarda o código de saída em uma variável global. */
    child_exit_status = status;
}

int split_command( char* cmd, char** arg_list ){
  
    int i = 0;
  
    for( i = 0; i < MAXPARAMS; i++ ) {
        arg_list[i] = strsep(&cmd, " ");
        if(arg_list[i] == NULL) break;
    }
  
    return 0;
}

int cmd_execute( char* program, char** arg_list ){
    
    char dir[MAXLEN];
    
    /* when 'cd' is called, a new thread-like is created */
    if ( strcmp ( program, "cd" ) == 0 ) {
        
        if ( strlen(arg_list[1]) == 0){
            chdir ( "/home/r2d2" );
        }
        else {
            
            getcwd(dir, sizeof(dir)); 
            
            if ( arg_list[1][0] != '/' ) strcat(dir, "/");
            
            strcat(dir, arg_list[1]); 
            
            if ( chdir ( dir ) == -1 )
                fprintf(stderr, "MyShell: cd: No such file or directory\n");
            
        }
        return 0;
    }
    /* otherwise it creates a process */
    else{
        int sp = 0; //stack pointer
        pid_t child_pid;
        child_pid = fork();
        
        
        if ( child_pid < 0 ) {
            printf("Error on fork: \n"); 
            return 1;
        }
        else if ( child_pid == 0 ) { // this is the child process
            
            if ( strcmp(program,"exit") == 0 ) {
                
                exit ( 0 );
                
            }
            else{
                
                //close ( pipe_fd[1] );
                //dup2 ( pipe_fd[0], STDOUT_FILENO );
                
                execvp ( program, arg_list );
                printf ( "Error: No such file or directory\n" );
            }
            
            exit(0);
        } 
        else { // this is the parent process
            
            int child_status;
            
            //close ( pipe_fd[0] );
            //dup2 ( pipe_fd[1], STDOUT_FILENO );
            
            waitpid ( child_pid, &child_status, 0 );
            return 1;
            
        }
    }
}

void prompt( char *username, char *hostname, char *new_cwd ){
  fprintf( stdout, "{MySh: %s@%s:%s}$ ", username, hostname, new_cwd );
}

char *abbreviate( char *input ){
    char *token, *temp, string[MAXLEN] = "";
    int i = 0;
  
    token = strtok(input, "/");
    
    if( strcmp(token, "home") != 0){
        strcat(string, token);
        strcat(string, "/");
    }else{    
        i++;
    }
    
    token = strtok(NULL, "/");
    
    
    if(strcmp(token, getenv ( "USER" ) ) == 0 ) i++;
    
    if(i == 2){
        strcat(string, "~");
    }
    
    
    do{

        token = strtok(NULL, "/");
        
        if(i == 1){
        i++;
        strcat(string, "/home");
        }

        if(token == NULL) break;
        
        if(token != NULL){
        
        strcat(string,"/");
        strcat(string,token);
        
        }
    }while(token != NULL);
    temp = string;
    return temp;
}

void piped(char* cmd){
    int i = 0;
    
    do{
        
        if(cmd == NULL) break;
        
        array[i] = strsep(&cmd, PIPESIG);
        i++; ap++;
        
    }while( i < ARRAYLIMIT );
}


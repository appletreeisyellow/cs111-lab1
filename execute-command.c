// UCLA CS 111 Lab 1 command execution

#include <alloc.h>
#include "command.h"
#include "command-internals.h"

#include <sys/types.h>
#include <sys/wait.h>

#include <error.h>
#include <sys/wait.h>	// waitpid()
#include <unistd.h>	// fork(), pipe(), execvp()
#include <stdio.h>	// freopen()
#include <stdlib.h>	// exit()
#include <string.h>     // strcpy(), strcat()

static int DEBUG_EXPAND;

char* my_string(char* first, char* second);
int command_status(command_t c);
void execute_pipe(command_t c);
void execute(command_t c);
void execute_sequence(command_t c);
int execute_command_type(command_t c);
void close_and_check(int pipe);


char* my_string(char* first, char* second)
{
    if (!(first && second)) return NULL;
    char* r = checked_malloc(sizeof(char) * (strlen(first) + strlen(second) + 1));
    strcpy(r, first);
    strcat(r, second);
    return r;
}


int
command_status (command_t c)
{
    return c->status;
}


void execute_pipe(command_t c)
{
    int child_status;
    pid_t left, right, return_pid;
    int mypipe[2];
    
    if(pipe(mypipe)<0)
    {
        fprintf(stderr, "ERROR: Creating pipe failed\n");
    }
    
    if((left=fork()) == -1){
        fprintf(stderr, "ERROR: Failed to create new process.\n");
    }
    else if( left == 0 )//left child
    {
        close_and_check(mypipe[0]);
        if(dup2(mypipe[1],1)==-1)
        {
            //check for error
            fprintf(stderr, "ERROR: Duplicating file descriptors failed.\n");
        }
        close_and_check(mypipe[1]);
        execute_command_type(c->u.command[0]);
        exit(c->u.command[0]->status);
        
    }
    else//parent
    {
        if((right=fork()) == -1){
            fprintf(stderr, "ERROR: Failed to create new process.\n");
        }
        else if(right == 0)//right child
        {
            close_and_check(mypipe[1]);
            if(dup2(mypipe[0],0)==-1)
            {
                //check for error
                fprintf(stderr, "ERROR: Duplicating file descriptors failed.\n");
            }
            close_and_check(mypipe[0]);
            execute_command_type(c->u.command[1]);
            exit(c->u.command[1]->status);
        }
        else//parent
        {
            close_and_check(mypipe[0]);
            close_and_check(mypipe[1]);
            return_pid=waitpid(-1,&child_status,0);
            
            if (return_pid==right)//wait for left
            {
                c->status=WEXITSTATUS(child_status);
                waitpid(left,&child_status,0);
            }
            else//wait for right
            {
                waitpid(right,&child_status,0);
                c->status=WEXITSTATUS(child_status);
            }
        }
    }
}



void execute(command_t c)
{
    pid_t cpid;
    
    if((cpid=fork()) == -1)
    {
        fprintf(stderr, "ERROR: Filed to create a new process.\n");
    }
    else if(cpid==0)//child
    {
        //open up file descriptors
        if(c->input != NULL){
            if (freopen(c->input, "r", stdin) == NULL){
                fprintf(stderr, "ERROR: File open failed.\n");
                exit(1);
            }
        }
        if(c->output != NULL){
            if(freopen(c->output, "w", stdout) == NULL){
                fprintf(stderr, "ERROR: File write failed.\n");
                exit(1);
            }
        }
        
        if (DEBUG_EXPAND)
        {
            char* cmd = "";
            int i = 0;
            while (c->u.word[i])
            {
                cmd = my_string(cmd, " ");
                cmd = my_string(cmd, c->u.word[i]);
                i++;
            }
            fprintf(stderr, "+%s\n", cmd);
        }
        
        if(execvp(c->u.word[0], c->u.word) < 0){
            fprintf(stderr, "ERROR: Failed to execute the command.\n");
            exit(1);
        }
        fclose(stdin);
        fclose(stdout);
    }
    else//parent
    {
        int child_status;
        waitpid(cpid, &child_status, 0);
        c->status = WEXITSTATUS(child_status);
        return;
    }
}

void execute_sequence(command_t c) {
    int child_status;
    pid_t left, right;
    
    if ((left = fork()) == -1) {
        fprintf(stderr, "ERROR: Failed to create new process.\n");
        exit(1);
    }
    else if (left == 0)//left child
    {
        execute_command_type(c->u.command[0]);
        exit(c->u.command[0]->status);
    }
    else//parent
    {
        waitpid(left, &child_status, 0);
        
        if ((right = fork()) == -1) {
            fprintf(stderr, "ERROR: Failed to create new process.\n");
            exit(1);
        }
        else if (right == 0)//right child
        {
            execute_command_type(c->u.command[1]);
            exit(c->u.command[1]->status);
        }
        else//parent
        {
            waitpid(right, &child_status, 0);
            c->status = WEXITSTATUS(child_status);
        }
    }
}

int execute_command_type(command_t c)
{
    switch(c->type){
            
        case SIMPLE_COMMAND:
            execute(c);
            break;
            
        case PIPE_COMMAND:
            execute_pipe(c);
            break;
            
        case AND_COMMAND:
            // the first command already fails
            if(execute_command_type(c->u.command[0]) != 0){
                c->status = command_status(c->u.command[0]);
            }else{
                c->status = execute_command_type(c->u.command[1]);
            }
            break;
            
        case OR_COMMAND:
            if(execute_command_type(c->u.command[0]) == 0){
                c->status = 0;
            }else{
                // try the second command if the first fails
                c->status = execute_command_type(c->u.command[1]);
            }
            break;
            
        case SUBSHELL_COMMAND:
            c->status = execute_command_type(c->u.subshell_command);
            break;
            
        case SEQUENCE_COMMAND: // return the status of the last command
            execute_sequence(c);
            break;
    }
    return c->status;
}

void close_and_check(int pipe){
    if(close(pipe) == -1){
        fprintf(stderr, "ERROR: Failed to close a file descriptor.\n");
        exit(1);
    }
}

void
execute_command (command_t c, int time_travel,int debug_expand)
{
    DEBUG_EXPAND=debug_expand;
    execute_command_type(c);
}

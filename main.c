// UCLA CS 111 Lab 1 main program

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>// waitpid()
#include <sys/types.h>// pid_t
#include <unistd.h>// fork()

extern void* allocate_address[1000000];
extern int total_allocated;
extern int v_enable;

static char const *program_name;
static char const *script_name;

//for 1c, dependency list and file list
extern struct dependency **dependency_list;
extern int num_tree;
extern char **file_list;
extern int num_file;

static void
usage (void)
{
    error (1, 0, "usage: %s [-pt] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
    return getc (stream);
}

// check if a command has dependency conflict with others
// one writer is allowed to write at most at each time
bool no_dependency(int cmd_num){
    int i,j;
    for(i=0;i<num_file;i++){
        // command 1 is writing to a file
        if(dependency_list[cmd_num][i].output>0){
            for(j=cmd_num-1;j>=0;j--){
                // command 2 cannot read or write to the same file concurrently
                if(dependency_list[j][i].input>0 || dependency_list[j][i].output>0)
                    return false;
            }
        }// command 1 is reading a file
        else if(dependency_list[cmd_num][i].input>0){
            for(j=cmd_num-1;j>=0;j--){
                // command 2 cannot write to the same file
                if(dependency_list[j][i].output>0)
                    return false;
            }
        }
    }
    return true;
}

// clear up the "traces" of a command on all files when it finishes
void clear_dependency(int cmd_num){
    
    int i;
    for(i=0;i<num_file;i++)
    {
        dependency_list[cmd_num][i].input=0;
        dependency_list[cmd_num][i].output=0;
    }
    return;
}

int
main (int argc, char **argv)
{
    int opt;
    int command_number = 1;
    int print_tree = 0;
    int time_travel = 0;
    int debug_regular = 0;
    int debug_expand = 0;
    program_name = argv[0];
	v_enable = 0;
    
    for (;;)
        switch (getopt (argc, argv, "ptvx"))
    {
        case 'p': print_tree = 1; break;
        case 't': time_travel = 1; break;
        case 'v': debug_regular = 1; v_enable = 1; break;
        case 'x': debug_expand = 1; break;
        default: usage (); break;
        case -1: goto options_exhausted;
    }
options_exhausted:;
    
    // There must be exactly one file argument.
    if (optind != argc - 1)
        usage ();
    
    script_name = argv[optind];
    FILE *script_stream = fopen (script_name, "r");
    if (! script_stream)
        error (1, errno, "%s: cannot open", script_name);
    command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);
    
    command_t last_command = NULL;
    command_t command;
    
    if(!time_travel){
        while ((command = read_command_stream (command_stream)))
        {
            if (print_tree)
            {
                printf ("# %d\n", command_number++);
                print_command (command);
            }
            else
            {
                last_command = command;
                if (debug_regular) {
                    putchar('\n');
                    print_regular(command);
                }
                execute_command(command, time_travel, debug_expand);
            }
        }
    }
    //time_travel case:
    else{
        // initialize a list of child processes for all command trees
        pid_t *child_proc_list = (pid_t*) checked_malloc(sizeof(pid_t) * num_tree);
        int i;
        for(i=0;i<num_tree;i++){
            child_proc_list[i]=0;
        }
        
        bool finished = false;
        
        while(!finished)
        {
            command_tree_t tmp=get_head(command_stream);
            int count=0;
            
            while(tmp){
                pid_t child_pid;
                
                if(child_proc_list[count]==0 && no_dependency(count)){
                    child_pid=fork();
                    
                    // child process: executes the command
                    if(child_pid==0)
                    {
                        execute_command (tmp->cmd, time_travel, debug_expand);
                        exit(0);
                    }
                    // parent process: collects child's pid
                    else if(child_pid>0){
                        child_proc_list[count]=child_pid;
                    }
                    else{
                        error(1,0, "fork() failed.");
                    }
                }
                tmp = tmp->next;
                count++;
            }
            
            for(i=0;i<num_tree;i++){
                if(child_proc_list[i]>0)
                {
                    int child_status;
                    // clear up the traces of the child on files once it finishes
                    waitpid(child_proc_list[i],&child_status,0);
                    clear_dependency(i);
                    child_proc_list[i]=-1;
                }
            }
            
            finished=true;
            for(i=0;i<num_tree;i++){
                if(child_proc_list[i]==0){
                    // there are remaining commands which haven't been executed
                    finished = false;
                }
            }
        }
        
        // free child process list
        free(child_proc_list);
        
        // free command trees
        while ((command = read_command_stream (command_stream))){}
        
        // free dependency list and file list
        int k=0;
        for(k=0;k<num_tree;k++){
            free(dependency_list[k]);
        }
        free(dependency_list);
        free(file_list);
        
    }
    
    int i;
    for (i = 0; i < total_allocated; i++)
        free(allocate_address[i]);
    
    return print_tree || !last_command ? 0 : command_status (last_command);
}

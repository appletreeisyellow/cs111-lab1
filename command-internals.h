// UCLA CS 111 Lab 1 command internals





enum command_type
{
    AND_COMMAND,         // A && B
    SEQUENCE_COMMAND,    // A ; B
    OR_COMMAND,          // A || B
    PIPE_COMMAND,        // A | B
    SIMPLE_COMMAND,      // a simple command
    SUBSHELL_COMMAND,    // ( A )
};

void* allocate_address[1000000];
int total_allocated;


// EXIST 0, READ 1, WRITE 2
struct dependency{
    int input;
    int output;
  };
  
struct dependency ** dependency_list;
int num_tree; // number of tree == number of cmd, each tree contains one cmd
char ** file_list;
int num_file;
  

// Data associated with a command.
struct command
{
    enum command_type type;
    
    // Exit status, or -1 if not known (e.g., because it has not exited yet).
    int status;
    
    // I/O redirections, or null if none.
    char *input;
    char *output;
    
    union
    {
        // for AND_COMMAND, SEQUENCE_COMMAND, OR_COMMAND, PIPE_COMMAND:
        struct command *command[2];
        
        // for SIMPLE_COMMAND:
        char **word;
        
        // for SUBSHELL_COMMAND:
        struct command *subshell_command;
    } u;
};

struct command_tree
{
    struct command* cmd;
    struct command_tree* next;
};


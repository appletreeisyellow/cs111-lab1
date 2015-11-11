// UCLA CS 111 Lab 1 command internals

static const char cmd_ls[] = "ls";
static const char cmd_cat[] = "cat";
static const char cmd_echo[] = "echo";
static const char cmd_rm[] = "rm";
static const char cmd_pwd[] = "pwd";
static const char cmd_sort[] = "sort";

enum command_type
{
    AND_COMMAND,         // A && B
    SEQUENCE_COMMAND,    // A ; B
    OR_COMMAND,          // A || B
    PIPE_COMMAND,        // A | B
    SIMPLE_COMMAND,      // a simple command
    SUBSHELL_COMMAND,    // ( A )
	COMMENT_COMMAND,	 // #...
};

enum command_name {
	LS,
	CAT,
	ECHO,
	RM,
	PWD,
	SORT,
	OTHERS,
};

void* allocate_address[1000000];
int total_allocated;
int v_enable;

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
    
    // Comments
    char* comment;
	// Line number
	int my_line_number;
};

struct command_tree
{
    struct command* cmd;
    struct command_tree* next;
};
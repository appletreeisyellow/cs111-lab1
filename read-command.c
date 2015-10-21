// UCLA CS 111 Lab 1 command reading
//modified by Kexin and Chunchun

#include "command.h"//make_command_stream, read_command_Stream
#include "command-internals.h"
#include "alloc.h"//checked_malloc

#include <stdio.h>//fprint
#include <stdlib.h>//exit
#include <string.h>//strcmp
#include <ctype.h>//isalpha, isdigit


///////////////////////////////////////////////////////////////////////////
// Global Variables
///////////////////////////////////////////////////////////////////////////
//keep track of the error line
int line_num;

//keep track of allocated memory, to be freed in main.c
extern void* allocate_address[1000000];
extern int total_allocated;

//keep track of '('
int operatorStack;


///////////////////////////////////////////////////////////////////////////
// Structure Declaration
///////////////////////////////////////////////////////////////////////////
//a linkede list of command tree node
struct command_stream
{
    struct command_tree* head;
    struct command_tree* tail;
};


///////////////////////////////////////////////////////////////////////////
// Function Declaration
///////////////////////////////////////////////////////////////////////////
bool isWord(char ch);
bool isSpecial(char ch);
bool isInvalid(char ch);
char the_next_non_space_tab_char(const char *s, int *p);

char* find_next_word(char *s, int *p, const int size);
void ignore_comment(char *s, int *p, const int size);

command_stream_t init_command_stream();
command_tree_t get_head(command_stream_t s);
command_t initiate_command_tree(char *s, int *p, const int size, bool sub_shell);

command_t initiate_command(enum command_type type);
command_t create_simple_command(char *s, int *p, const int size);
command_t create_special_command(char *s, int *p, const int size, enum command_type type, command_t left, bool is_in_sub_shell);


///////////////////////////////////////////////////////////////////////////
// Function Implementation
///////////////////////////////////////////////////////////////////////////
bool isWord(char ch)
{
    return isalpha(ch) || isdigit(ch) || (ch == '!') || (ch == '%') || (ch == '+') || (ch == ',') || (ch == '-') || (ch == '.') || (ch == '/') || (ch == ':') || (ch == '@') || (ch == '^') || (ch == '_');
}


bool isSpecial(char ch)
{
    return (ch == '|' || ch == '&' || ch ==';'|| ch ==')' || ch =='(');
}


bool isInvalid(char ch)
{
    return !isWord(ch) && !isSpecial(ch) && ch!='\n' && ch!=' ' && ch!='#';
}

// Return the next first non-space non-tab char
// Usually use this function to check if a special token starts at the beginning of a line
char the_next_non_space_tab_char(const char *s, int *p){
	int * copyed_p = p;
	do{
		(*copyed_p)++;
	}while(s[*copyed_p] == ' ' || s[*copyed_p] == '\t');
	
	return s[*copyed_p];
}

// Get the next string of characters
char* find_next_word(char *s, int *p, const int size)
{
    int begining; // To remember the first char of the next ordinary word
    int word_len;
    char *word;
    
    // Escape space, tab, and comments until p points to the next ordinary word
    while( (s[*p] == ' ' || s[*p] == '\t' || s[*p] == '#') && (*p < size) ){
        if( s[*p] == '#')
        {
            ignore_comment(s, p, size);
        }
        (*p)++;
    }
    
    begining = *p;
    
    while( (*p < size) && isWord(s[*p]) )
    {
        (*p)++;
    }
    
    word_len = (*p) - begining;
    if( isSpecial(s[*p]) || s[*p] == '<' || s[*p] == '>' ||
       s[*p] == '\n' || s[*p] == '#')
    {
        (*p)--;
    }
    
    if(word_len == 0)
    {
        return NULL;
    }
    
    word = (char*) checked_malloc( (word_len + 1) * sizeof(char) );
	allocate_address[total_allocated] = word;
	total_allocated++;
    
    // Copy the string of char into word
    
    int i;
    for(i = 0; i < word_len; i++)
    {
        word[i] = s[begining + i];
    }
    
    word[word_len] = '\0';
    return word;
}


void ignore_comment(char *s, int *p, const int size)//Kexin
{
    //find the next new line
    for(;*p<size; (*p)++){
        if(s[*p] == '\n'){
            line_num++; //printf("line %d\n ignore_comment", line_num);
            return;
        }
    }
}


command_stream_t init_command_stream()
{
    command_stream_t new_cmd_strm = (command_stream_t) checked_malloc(sizeof(struct command_stream));
	allocate_address[total_allocated] = new_cmd_strm;
	total_allocated++;
	new_cmd_strm->head = NULL;
    new_cmd_strm->tail = NULL;
    return new_cmd_strm;
}

command_tree_t get_head(command_stream_t s){
  if(s){
    return s->head;
  }
  return NULL;
}
    

command_t initiate_command_tree(char *s, int *p, const int size, bool sub_shell)//Kexin
{
    //start with the leftmost command in the tree
    command_t left = NULL; 
    
    for(; *p<size; (*p)++){
        char current_char = s[*p];
        
        //skip the space
        if(current_char==' ' || current_char=='\t'){
            continue;
        }
        //newline
        else if(current_char=='\n'&&(*p)!=size-1){
			char next_char = s[*p + 1];
			if ( next_char == '\n' && !sub_shell) //may have error here, what if \n is inbetween && and next command
			{
				if(left)
				{
					line_num++; //printf("line %d in newline !sub_shell left\n", line_num);
					break;
				}
				else 
				{
					line_num++; //printf("line %d newline !sub_shell else\n", line_num);
					continue;
				}
			}
			else if( next_char == '\n' && sub_shell){
				line_num++; //printf("line %d newline sub_shell\n", line_num);
				continue;
			}
			else if(next_char == '(' || next_char == ')' || next_char == '#')
			{
				line_num++; //printf("line %d ()#\n", line_num);
				continue;
			}
			else if(isWord(next_char) && left)
			{
				(*p) += 1;
				line_num++; //printf("line %d isWord left\n", line_num);
				left = create_special_command(s, p, size, SEQUENCE_COMMAND, left, sub_shell);
			}
			else if (isWord(next_char) && !left)
			{
				line_num++; //printf("line %d isWord !left", line_num);
				continue;
			}
            else{
				char next_first_nonspacetab_char = the_next_non_space_tab_char(s, p);
				if(next_first_nonspacetab_char == ';' || next_first_nonspacetab_char == '|' 
					|| next_first_nonspacetab_char == '&' || next_first_nonspacetab_char == '<' || next_first_nonspacetab_char == '>' )
				{
					fprintf(stderr, "Line %d: New line cannot appear before %c\n", line_num, next_first_nonspacetab_char);
					exit(1);
				}
				else
				{
					line_num++; //printf("line %d else else \n", line_num);
					continue;
				}
				
            }
        }
        //simple command
        else if(isWord(current_char)){
            left=create_simple_command(s,p,size);
        }
        //invalid expression
        else if(isInvalid(current_char)){

			if(sub_shell){
			
			if(current_char == '<')
			{
				if (left->input){
					fprintf(stderr, "Line %d: Already exist an I/O input.\n", line_num);
					exit(1);
				}
				(*p)++;
				left->input = find_next_word(s, p, size);
				// Check if successfully find the next word
				if (left->input == NULL){
					fprintf(stderr, "Line %d: No where to input.\n", line_num);
					exit(1);
				}
			}
			else if (current_char == '>'){
				if (left->output){
					fprintf(stderr, "Line %d: Already exist an I/O output.\n", line_num);
					exit(1);
				}
				(*p)++;
				left->output = find_next_word(s, p, size);
				if (left->output == NULL)
				{
					fprintf(stderr, "Line %d: No where to output.\n", line_num);
					exit(1);
				}
			}

			}
			else{
            fprintf(stderr,"Line %d: Invalid expression '%c'.\n", line_num, current_char);
            exit(1);}
        }
        //command completed by a semicolon
        else if(current_char==';'){
            //no command found before the semicolon
            if(left==NULL){
                fprintf(stderr,"Line %d: ';' should not come alone.\n", line_num);
                exit(1);
            }
            //handle sequence command in a subshell
            if(sub_shell){
				//skip this ';'
                (*p)+=1;
                left=create_special_command(s, p, size, SEQUENCE_COMMAND, left, sub_shell);
            }
            else{
				(*p) += 1;
				left = create_special_command(s, p, size, SEQUENCE_COMMAND, left, sub_shell);
                //break;
            }
        }
        else if(current_char=='&'){
            if(*p+1<size&&s[*p+1]=='&'){
				if (left == NULL) {
					fprintf(stderr, "Line %d: '&&' should not be the start of a command.\n", line_num);
					exit(1);
				}
                //valid AND command, skip this '&&'
				(*p) += 2;
                left=create_special_command(s, p, size, AND_COMMAND, left, sub_shell);
            }
            else{
                //invalid single '&'
				fprintf(stderr,"Line %d: Invalid expression '%c'.\n", line_num,current_char);
                exit(1);
            }
        }
        else if(current_char=='|'){
            if(*p+1<size&&s[*p+1]=='|'){
				if (left == NULL) {
					fprintf(stderr, "Line %d: '||' should not be the start of a command.\n", line_num);
					exit(1);
				}
                //valid OR command, skip this '||'
				(*p) += 2;
                left=create_special_command(s, p, size, OR_COMMAND, left, sub_shell);
            }
            else{
				if (left == NULL) {
					fprintf(stderr, "Line %d: Pipe '|' should not be the start of a command.\n", line_num);
					exit(1);
				}
                //valid PIPE command, skip this '|'
				(*p) += 1;
                left=create_special_command(s, p, size, PIPE_COMMAND, left, sub_shell);
            }
        }
        else if(current_char=='('){
			operatorStack++;
            //SUBSHELL command, skip this '('
            left= initiate_command(SUBSHELL_COMMAND);
			(*p) += 1;
            left->u.subshell_command = initiate_command_tree(s, p, size, true);
        }
        else if(current_char==')'){

			char next_char = s[*p+1];
			
			if (next_char == ' ') {
				while (next_char == ' ') {
					(*p) += 1;
					next_char = s[*p + 1];
				}

				(*p) += 1;
				current_char = s[*p];

				if (sub_shell) {
					if (current_char == '<')
					{
						if (left->input) {
							fprintf(stderr, "Line %d: Already exist an I/O input.\n", line_num);
							exit(1);
						}
						(*p)++;
						left->input = find_next_word(s, p, size);
						// Check if successfully find the next word
						if (left->input == NULL) {
							fprintf(stderr, "Line %d: No where to input.\n", line_num);
							exit(1);
						}
					}
					else if (current_char == '>') {
						if (left->output) {
							fprintf(stderr, "Line %d: Already exist an I/O output.\n", line_num);
							exit(1);
						}
						(*p)++;
						left->output = find_next_word(s, p, size);
						if (left->output == NULL)
						{
							fprintf(stderr, "Line %d: No where to output.\n", line_num);
							exit(1);
						}
					}

				}
				else {
					fprintf(stderr, "Line %d: Invalid expression '%c'.\n", line_num, current_char);
					exit(1);
				}
			}



			if(sub_shell){
				operatorStack -= 1;;
				if(operatorStack==0)
					return left;
			}
			else{
				//unmatched 
				fprintf(stderr, "Line %d: Unmatched ')' character.\n", line_num);
				exit(1);
			}
        }
        else if(current_char=='#'){
            //ignore comments
            ignore_comment(s, p, size);
        }
    }//end of for loop
    
	//unmatched '('
	if (operatorStack>0) {
		fprintf(stderr, "Line %d: Unmatched '(' character.\n", line_num);
		printf("operatorStack: %d\n", operatorStack);
		exit(1);
	}
	//unmatched ')'
	else if (operatorStack < 0)
	{
		fprintf(stderr, "Line %d: Unmatched ')' character.\n", line_num);
		exit(1);
	}
    return left;
}


command_t initiate_command(enum command_type type)
{
    command_t new_cmd = (command_t) checked_malloc(sizeof(struct command));
	allocate_address[total_allocated]=new_cmd;
	total_allocated++;
	new_cmd->type = type;
    new_cmd->status = -1;
    new_cmd->input = NULL;
    new_cmd->output = NULL;
    return new_cmd;
}


command_t create_simple_command(char *s, int *p, const int size)
{
    command_t cmd = initiate_command(SIMPLE_COMMAND);
    // cmd->u.word points to an array of char pointers
    // The last cell of array points to NULL
    int array_index = 0;
    int array_size = 1;
    cmd->u.word = (char**) checked_malloc( sizeof(char*) * array_size);
    
	allocate_address[total_allocated] = cmd->u.word;
	total_allocated++;

    for(; *p < size; (*p)++)
    {
        char current_char = s[*p];
        
        if( isWord(current_char) )
        {
            char *word = find_next_word(s, p, size);// May have error here
            if( array_index == array_size)
            {
                array_size *= 2;
                cmd->u.word = checked_realloc( cmd->u.word, sizeof(char*) * array_size);
            }
            
            cmd->u.word[array_index++] = word;
            cmd->u.word[array_index] = NULL;
        }
        else if( current_char == ' ' || current_char == '\t') { continue; }
        else if( current_char == '#' ) { ignore_comment(s, p, size); }
        else if( current_char == '<' )
        {
            if(cmd->input)
            {
                fprintf(stderr, "Line %d: Already exist an I/O input.\n", line_num);
                exit(1);
            }
            (*p)++;
            cmd->input = find_next_word(s, p, size);
            // Check if successfully find the next word
            if(cmd->input == NULL)
            {
                fprintf(stderr, "Line %d: No where to input.\n", line_num);
                exit(1);
            }
        }
        else if( current_char == '>' )
        {
            if(cmd->output)
            {
                fprintf(stderr, "Line %d: Already exist an I/O output.\n", line_num);
                exit(1);
            }
            (*p)++;
            cmd->output = find_next_word(s, p, size);
            if(cmd->output == NULL)
            {
                fprintf(stderr, "Line %d: No where to output.\n", line_num);
                exit(1);
            }
        }
        else if( current_char == '\n')
        {
			line_num++; //printf("line %d", line_num);
            (*p)++;
            break;
        }
		else if (isInvalid(current_char))
		{
			fprintf(stderr, "Line %d: invalid character '%c'.\n", line_num, current_char);
			exit(1);
		}
        else if( isSpecial(current_char)) 
        { 
            (*p)++; 
            break; 
        } 
    }// end of for loop
    
    // Let p points to the last char of the SIMPLE_COMMAND
    (*p) -= 2;
    return cmd;
}



command_t create_special_command(char *s, int *p, const int size, enum command_type type, command_t left, bool is_in_sub_shell)//Kexin
{
    //initiate the command
    command_t cmd = initiate_command(type);
    
    //pipe command
    if(type == PIPE_COMMAND && (left->type == AND_COMMAND || left->type == OR_COMMAND)){
        cmd->u.command[0] = left->u.command[1];
        left->u.command[1] = cmd;
    }else{
        cmd->u.command[0] = left;
    }
    
    for(; *p<size; (*p)++){
        char current_char = s[*p];
        //skip the space
        if(current_char==' ' || current_char=='\t'){
            continue;
            
        }//newline
        else if(current_char=='\n'&&(*p)!=size-1){
            line_num++; //printf("line %d", line_num);
            continue;
        }
        //complete the right command branch
        else if(isWord(current_char)){
			if(type == SEQUENCE_COMMAND)
				cmd->u.command[1] = initiate_command_tree(s, p, size,is_in_sub_shell);
			else
				cmd->u.command[1] = create_simple_command(s, p, size);

            if(type == PIPE_COMMAND && (left->type == AND_COMMAND || left->type == OR_COMMAND)){
                return left;
            }
            return cmd;
        }
        //invalid expression
        else if(isInvalid(current_char)){
            fprintf(stderr,"Line %d: Invalid expression '%c'.\n", line_num, current_char);
            exit(1);
        }
        //subshell
        else if(current_char == '('){
            //(*p)+=1;
            command_t right = initiate_command(SUBSHELL_COMMAND);
            right->u.subshell_command = initiate_command_tree(s, p, size, true);
            cmd->u.command[1] = right;
            return cmd;
        }
		else if (current_char == ';' || current_char == '|' || current_char == '&' || current_char == ')') {
			fprintf(stderr, "Line %d: Invalid character '%c' following the special command.\n", line_num,current_char);
			exit(1);
			//added skip comment
		}
        //ignore comments
        else if(current_char == '#'){
            ignore_comment(s, p, size);
        }
    }
    //no following command found
	if (type != SEQUENCE_COMMAND)
	{
		fprintf(stderr, "Line %d: There should be a following command.\n", line_num);
		exit(1);
	}
	return cmd;
}


command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
    //initiate a buffer
    size_t bufferSize = 1000;
    size_t checked = 0;
    int c;
    char* buffer = (char*) checked_malloc(bufferSize);
    
    //read bytes into the buffer and increase its size if needed
    while((c = get_next_byte(get_next_byte_argument))!= EOF){
		
		buffer[checked++] = c;
		
		if(checked == bufferSize){
			bufferSize *=2;
			buffer = (char*) checked_grow_alloc(buffer, &bufferSize);
		}
    }
	
	if( checked == 0 ){
		fprintf(stderr, "Nothing is in the opened file.\n");
		exit(1);
	}
	
    //initiate a linked list of command trees
    command_stream_t stream= init_command_stream();
    
    int i;
    int size = (int) checked;
    line_num = 0;
	operatorStack = 0;
    
    for(i = 0; i < size; i++){
        //initiate a command tree
	command_t new_command_tree=initiate_command_tree(buffer,&i,size,false);
        
        if(new_command_tree){
            //create a new command tree node in the linked list
            command_tree_t new_tree = (command_tree_t) checked_malloc(sizeof(struct command_tree));
			
			allocate_address[total_allocated] = new_tree;
			total_allocated++;

			new_tree->next = NULL;
            new_tree->cmd = new_command_tree;
            //first command tree in the stream
            if(!stream->head){
                stream->head = new_tree;
                stream->tail = new_tree;
            }else{
                stream->tail->next = new_tree;
                stream->tail = new_tree;
            }
        }
    }
    free(buffer);
    return stream;
}


command_t
read_command_stream (command_stream_t s)
{
    if(s->head){
        //clear the command tree from the stream after reading it
        command_t new_cmd = s->head->cmd;
        command_tree_t temp = s->head;
        s->head = s->head->next;
        free(temp);
        return new_cmd;
    }
    return NULL;
}

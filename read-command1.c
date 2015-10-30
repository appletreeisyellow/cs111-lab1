// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <error.h>
#include <stdlib.h>


/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
struct stream_node {
  struct command *cmd;
  struct stream_node *next;
};

struct command_stream {
  struct stream_node *head;
  struct stream_node *tail;
};



command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  size_t bufferSize = 1000000;
  size_t checked = 0;
  int c;
  char * buffer = (char*) checked_malloc(bufferSize);
}

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
   an error, report the error and exit instead of returning.  */
command_t
read_command_stream (command_stream_t s)
{
	/* 	If the stream contains command trees, copy and delete the first 
		command tree (node). */
	if(s->head){
		command_t	cmd_to_read		= s->head->cmd;
		stream_node	node_to_delete 	= s->head;
		s->head = s->head->next;
		free(node_to_delete);
		return node_to_read;
	}
  
	return NULL;
}

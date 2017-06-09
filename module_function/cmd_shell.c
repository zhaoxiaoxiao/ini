
#include <termios.h>

#include "common.h"
#include "cmd_shell.h"
#include "frame_tool.h"


#define CMD_SHELL_ENDLINE_SIGN		10

#define CMD_SHELL_ARRAY_32			32
#define CMD_SHELL_ARRAY_64			64
#define CMD_SHELL_ARRAY_128			128

#define CMD_SHELL_LINE_MAXSIZE		4096

char user_name[CMD_SHELL_ARRAY_64] = {0},local_folder[CMD_SHELL_ARRAY_64] = {0},curr_path[CMD_SHELL_ARRAY_128] = {0};

void cmd_shell_input(void *data)
{
	char *p_char = NULL, *q_char = NULL,input_buf[CMD_SHELL_LINE_MAXSIZE] = {0};

	p_char = current_user_name(user_name,CMD_SHELL_ARRAY_64);
	if(p_char == NULL)
	{
		return;
	}
	while(1)
	{
		printf("[%s@localhost thread_pool]#",user_name);
		p_char = fgets(input_buf,CMD_SHELL_LINE_MAXSIZE,stdin);
		if(p_char)
		{
			q_char = framestr_last_constchar(p_char,CMD_SHELL_ENDLINE_SIGN);
			if(q_char)
				*q_char = 0;
			printf("-bash: %s: command not found",p_char);
		}
		printf("\n");
		memset(input_buf,0,CMD_SHELL_LINE_MAXSIZE);
	}
}


void cmd_shell_destroy()
{
	printf("\n");
}

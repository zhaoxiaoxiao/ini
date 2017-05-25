
#include "cmd_shell.h"
#include "common.h"

void sig_catch(int sig)
{
	switch(sig)
	{
		case SIGALRM:
			break;
		case SIGINT:
			break;
		case SIGSEGV:
			break;
		case SIGTERM:
			break;
		default:
			break;
	}
	cmd_shell_destroy();
	PERROR("!!!!!!!!well, we catch signal in this process :::%d\n\n",sig);
	exit(0);
	return;
}

int main(int argc, char *argv[])
{

	(void)signal(SIGALRM,sig_catch);//timer
	(void)signal(SIGINT, sig_catch);//ctrl + c 
	(void)signal(SIGSEGV, sig_catch);//memory
	(void)signal(SIGTERM, sig_catch);//kill

	cmd_shell_input(NULL);
	return 0;
}



#include "cmd_shell.h"
#include "common.h"
#include "log_mini.h"
#include "lottery.h"
#include "es_http_api.h"

void sig_catch(int sig)
{
	switch(sig)
	{
		case SIGALRM:
			PERROR("SIGALRM :: %d\n",SIGALRM);
			break;
		case SIGINT:
			PERROR("SIGINT :: %d\n",SIGINT);
			break;
		case SIGSEGV:
			PERROR("SIGSEGV :: %d\n",SIGSEGV);
			break;
		case SIGTERM:
			PERROR("SIGTERM :: %d\n",SIGTERM);
			break;
		default:
			PERROR("%d\n",sig);
			break;
	}
	
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

	es_server_init("127.0.0.1",50001,HTTP);
	es_server_query(GET_METHOD,"/elefence_terminal_info/_search","pretty","{\"query\":{\"term\":{\"terminal_mac\":\"E8-3A-12-B0-6C-DB\"}}}");
	es_server_destroy();
	return 0;
}


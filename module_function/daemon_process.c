
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"
#include "daemon_process.h"

int daemon_process_init()
{
    pid_t pid;

	pid=fork();
    if(pid < 0)
		return -1;
    else if(pid != 0)
		exit(0);/*parentexit*/

	/*childcontinues*/
	setsid();/*becomesessionleader*/
    chdir("/");/*changeworkingdirectory*/
    umask(0);/*clearfilemodecreationmask*/

	close(0);/*closestdin*/
    close(1);/*closestdout*/
    close(2);/*closestderr*/

	return 0;
}


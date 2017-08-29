
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

int daemonize(int nochdir, int noclose)
{
    int fd;

    switch (fork()) {
    case -1:
        return (-1);
    case 0:
        break;
    default:
        _exit(EXIT_SUCCESS);
    }

    if (setsid() == -1)
        return (-1);

    if (nochdir == 0) {
        if(chdir("/") != 0) {
            perror("chdir");
            return (-1);
        }
    }

    if (noclose == 0 && (fd = open("/dev/null", O_RDWR, 0)) != -1) {
        if(dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2 stdin");
            return (-1);
        }
        if(dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 stdout");
            return (-1);
        }
        if(dup2(fd, STDERR_FILENO) < 0) {
            perror("dup2 stderr");
            return (-1);
        }

        if (fd > STDERR_FILENO) {
            if(close(fd) < 0) {
                perror("close");
                return (-1);
            }
        }
    }
    return (0);
}


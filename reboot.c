#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

int
main(void)
{
#ifdef SANE
	if (kill(1, SIGINT) == -1) {
		perror("kill");
		return errno;
	}
#else
        kill(1, SIGINT);
#endif

	return 0;
}

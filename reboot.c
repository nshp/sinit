#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

int
main(void)
{
	if (kill(1, SIGINT) == -1) {
		perror("kill");
		return errno;
	}

	return 0;
}

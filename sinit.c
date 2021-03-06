/* See LICENSE file for copyright and license details. */
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <net/if.h>

#include <fcntl.h>
#include <grp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>

#define LEN(x) (sizeof (x) / sizeof *(x))

static void sigpoweroff(void);
static void sigreap(void);
static void sigreboot(void);
static void spawn(int (*)(), char *const []);
static void spawn_as(uid_t, gid_t, char *const [], char *const []);
static void mounts(void);
static void ifup(const char *iface);

static struct {
	int sig;
	void (*handler)(void);
} sigmap[] = {
	{ SIGUSR1, sigpoweroff },
	{ SIGCHLD, sigreap     },
	{ SIGINT,  sigreboot   },
};

#include "config.h"

static sigset_t set;

int
main(void)
{
	int sig;
	size_t i;

#ifdef SANE
	if (getpid() != 1)
		return 1;

	chdir("/");
#endif

#ifdef PERF
	spawn(NULL, perf);
#endif

	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);

	sethostname(HOSTNAME, sizeof(HOSTNAME)-1);
	mounts();
	ifup("lo");

	ioctl(STDIN_FILENO, TIOCNOTTY, 0);

	for (i=0;i < LEN(init_procs);i++) {
		spawn(init_procs[i].dep, init_procs[i].argv);
	}
	for (i=0;i < LEN(user_procs);i++) {
		spawn_as(uid, gid, user_env, user_procs[i]);
	}
	while (1) {
		sigwait(&set, &sig);
		for (i = 0; i < LEN(sigmap); i++) {
			if (sigmap[i].sig == sig) {
				sigmap[i].handler();
				break;
			}
		}
	}
	/* not reachable */
	return 0;
}

static void
kill_wait(void)
{
	int i;

	for (i=0;i < 4 && kill(-1, i>=3 ? SIGKILL:SIGTERM) == 0;i++) {
		/* printf("killall %d\n", i); */
		sleep(1);
	}
}

static void
sigpoweroff(void)
{
	sigprocmask(SIG_BLOCK, &set, NULL);
	kill_wait();
	sync();
	reboot(RB_POWER_OFF);
#ifdef SANE
	/* only reachable on error */
	write(2, "poweroff failed\n", 16);
#endif
}

static void
sigreap(void)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		;
}

static void
sigreboot(void)
{
	sigprocmask(SIG_BLOCK, &set, NULL);
	kill_wait();
	sync();
	reboot(RB_AUTOBOOT);
#ifdef SANE
	/* only reachable on error */
	write(2, "reboot failed\n", 15);
#endif
}

static void
spawn(int (*dep)(), char *const argv[])

{
	struct timespec delay = {
		.tv_nsec = 1000,
		.tv_sec = 0,
	};

	switch (fork()) {
	case 0:
		sigprocmask(SIG_UNBLOCK, &set, NULL);
		setsid();
		if (dep != NULL) {
			while (!dep()) nanosleep(&delay, NULL);
		}
		/* printf("spawn: %s\n", argv[0]); */
		execvp(argv[0], argv);
#ifdef SANE
		write(2, "can't exec\n", 11);
		_exit(1);
	case -1:
		write(2, "can't fork\n", 11);
#endif
	}
}

static void
spawn_as(uid_t uid, gid_t gid, char *const env[], char *const argv[])
{
	/* printf("spawn_as %d,%d: %s\n", uid, gid, argv[0]); */
	switch (fork()) {
	case 0:
		chdir(HOMEDIR);
		sigprocmask(SIG_UNBLOCK, &set, NULL);
		setsid();
		ioctl(STDIN_FILENO, TIOCSCTTY, 0);
		setgid(gid);
		setgroups(LEN(groups), groups);
		setuid(uid);
		execve(argv[0], argv, env);
#ifdef SANE
		write(2, "can't exec\n", 11);
		_exit(1);
	case -1:
		write(2, "can't fork\n", 11);
#endif
	}
}

static void
mounts(void)
{
	size_t i;
	mkdir("/dev/pts", 0755);
	mkdir("/dev/shm", 0755);
	for (i=0;i < LEN(static_mounts);i++) {
		if (mount(static_mounts[i].source,
			  static_mounts[i].target,
			  static_mounts[i].type,
			  static_mounts[i].flags,
			  static_mounts[i].data) != 0)
		{
#ifdef SANE
			write(2, "mount failed for ", 17);
			write(2, static_mounts[i].target,
					strlen(static_mounts[i].target));
			write(2, "\n", 1);
#endif
		}

	}
}

int
udev_settled(void)
{
	struct stat info;
	if (stat("/dev/input/event4", &info))
		return 0;
	return info.st_gid == 97; /* input group */
}

static void
ifup(const char *iface)
{
	struct ifreq ifr;
	int sk;

	sk = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef SANE
	if (sk < 0) {
		write(2, "ifup failed for ", 16);
		write(2, iface, strlen(iface));
		write(2, "\n", 1);
		return;
	}
#endif

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, iface, IFNAMSIZ);
	ifr.ifr_flags |= IFF_UP;
	ioctl(sk, SIOCSIFFLAGS, &ifr);
}

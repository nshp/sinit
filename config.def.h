/* See LICENSE file for copyright and license details. */

/* user/group ID for user processes */
#define HOSTNAME "what"
#define USERNAME "who"
#define HOMEDIR  "/home/" USERNAME

uid_t uid = 1000;
gid_t gid = 1000;

/* Enable startup profiling with perf(1) */
// #define PERF

#ifdef PERF
static char *const perf[] = {
      	"/usr/bin/perf", "record", "-ag", "--output=/tmp/perf.data",
       	"--", "sleep", "3" };
#endif

/* constant groups to set for user session */
static gid_t const groups[] = {
	5, 	/* tty */
	10,	/* wheel */
	91, 	/* video */
	92, 	/* audio */
};

int wpa_started(void);
int udev_settled(void);

static struct {
	int (*dep)();
	char *argv[6];
} init_procs[] = {
	NULL, 		{ "/usr/bin/agetty", "-p", "tty2", "linux", NULL, NULL },
	NULL, 		{ "/usr/bin/spm", NULL, NULL, NULL, NULL, NULL },
	NULL, 		{ "/usr/bin/udevd", "--daemon", NULL },
	NULL, 		{ "/usr/bin/udevadm", "trigger", "-c", "add", NULL },
	NULL, 		{ "/usr/bin/udevadm", "settle", NULL },
	NULL, 		{ "/usr/bin/wpa_supplicant", "-iwlan0", "-c", "/etc/wpa_supplicant/wpa_supplicant.conf", NULL },
	NULL, 		{ "/usr/bin/thermald", NULL },
	wpa_started,  	{ "/usr/bin/wpa_cli", "-a", "/usr/local/bin/run_dhcpcd.sh", NULL },
	wpa_started,  	{ "/usr/bin/dhcpcd", "wlan0", NULL },
	udev_settled, 	{ "/usr/bin/illum-d", NULL },
};

static struct {
	char *source;
	char *target;
	char *type;
	unsigned long flags;
	char *data;
} static_mounts[] = {
	{ "tmpfs",  "/tmp",     "tmpfs",  MS_NOSUID|MS_NODEV,                       "" },
	{ "sysfs",  "/sys",     "sysfs",  MS_NOSUID|MS_NODEV|MS_NOEXEC|MS_RELATIME, "rw" },
	{ "proc",   "/proc",    "proc",   MS_NOSUID|MS_NODEV|MS_NOEXEC|MS_RELATIME, "rw" },
	{ "devpts", "/dev/pts", "devpts", MS_NOSUID|MS_NOEXEC|MS_RELATIME,          "gid=5,mode=620,ptmxmode=000" },
	{ "tmpfs",  "/dev/shm", "tmpfs",  MS_NOSUID|MS_NODEV,                       "mode=1777" },
	{ "tmpfs",  "/run",     "tmpfs",  MS_NOSUID|MS_NODEV,                       "mode=755" },
};

static char *const user_env[] = {
	"HOME=" HOMEDIR,
	"PWD=" HOMEDIR,
	"PATH=/usr/bin",
	"DISPLAY=:0",
	"LANG=en_US.UTF-8",
	"SHELL=/bin/bash",
	"USER=" USERNAME,
	"LOGNAME=" USERNAME,
	NULL
};

static char *const user_procs[][5]  = {
	{ "/usr/bin/xinit", "/usr/bin/dwm", "--", "-quiet", NULL },
};


#include <time.h>
#include <errno.h>
#include <sys/stat.h>

static struct timespec last_modified(char const *filename) {
#if __unix__
	struct stat statbuf = {0};
	stat(filename, &statbuf);
	return statbuf.st_mtim;
#else
	// windows' _stat does not have st_mtim
	struct _stat statbuf = {0};
	struct timespec ts = {0};
	_stat(filename, &statbuf);
	ts.tv_sec = statbuf.st_mtime;
	return ts;
#endif
}

static int timespec_cmp(struct timespec a, struct timespec b) {
	if (a.tv_sec  > b.tv_sec)  return 1;
	if (a.tv_sec  < b.tv_sec)  return -1;
	if (a.tv_nsec > b.tv_nsec) return 1;
	if (a.tv_nsec < b.tv_nsec) return -1;
	return 0;
}

// sleep for a certain number of nanoseconds
static void sleep_ns(u64 ns) {
#if __unix__
	struct timespec rem = {0}, req = {
		ns / 1000000000,
		ns % 1000000000
	};
	
	while (nanosleep(&req, &rem) == EINTR) // sleep interrupted by signal
		req = rem;
#else
	// windows....
	Sleep((DWORD)(ns / 1000000));
#endif
}

// sleep for microseconds
static void sleep_us(u64 us) {
	sleep_ns(us * 1000);
}

// sleep for milliseconds
static void sleep_ms(u64 ms) {
	sleep_ns(ms * 1000000);
}

// sleep for seconds
static void sleep_s(u64 s) {
	sleep_ns(s * 1000000000);
}

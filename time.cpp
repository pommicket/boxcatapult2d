#include <time.h>
#include <errno.h>
#include <sys/stat.h>

static struct timespec time_last_modified(char const *filename) {
#if __unix__
	struct stat statbuf = {};
	stat(filename, &statbuf);
	return statbuf.st_mtim;
#else
	// windows' _stat does not have st_mtim
	struct _stat statbuf = {};
	struct timespec ts = {};
	_stat(filename, &statbuf);
	ts.tv_sec = statbuf.st_mtime;
	return ts;
#endif
}

// get the current time. this should only really be used for
// time intervals, since there's no significance to the absolute number.
static struct timespec time_get(void) {
	struct timespec ts = {};
#if _WIN32
	FILETIME ft;
	ULARGE_INTEGER bigint;
	ULONGLONG t;
	GetSystemTimeAsFileTime(&ft);
	bigint.LowPart = ft.dwLowDateTime;
	bigint.HighPart = ft.dwHighDateTime;
	t = bigint.QuadPart;
	ts.tv_sec = t / 10000000;
	ts.tv_nsec = (t % 10000000) * 100;
#else
	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
	return ts;
}

// subtract timespecs. the return value is seconds.
static double timespec_sub(struct timespec a, struct timespec b) {
	return (double)(a.tv_sec - b.tv_sec)
		+ 0.000000001 * (double)(a.tv_nsec - b.tv_nsec);
}

static int timespec_cmp(struct timespec a, struct timespec b) {
	if (a.tv_sec  > b.tv_sec)  return 1;
	if (a.tv_sec  < b.tv_sec)  return -1;
	if (a.tv_nsec > b.tv_nsec) return 1;
	if (a.tv_nsec < b.tv_nsec) return -1;
	return 0;
}

static bool timespec_eq(struct timespec a, struct timespec b) {
	return timespec_cmp(a, b) == 0;
}

static struct timespec timespec_max(struct timespec a, struct timespec b) {
	return timespec_cmp(a, b) < 0 ? b : a;
}

// sleep for a certain number of nanoseconds
static void sleep_ns(u64 ns) {
#if __unix__
	struct timespec rem = {}, req = {
		(time_t)(ns / 1000000000),
		(long)(ns % 1000000000)
	};
	
	while (nanosleep(&req, &rem) == EINTR) // sleep interrupted by signal
		req = rem;
#else
	// windows....
	Sleep((DWORD)(ns / 1000000));
#endif
}

// sleep for microseconds
static void time_sleep_us(u64 us) {
	sleep_ns(us * 1000);
}

// sleep for milliseconds
static void time_sleep_ms(u64 ms) {
	sleep_ns(ms * 1000000);
}

// sleep for seconds
static void time_sleep_s(u64 s) {
	sleep_ns(s * 1000000000);
}

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/resource.h>

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		fprintf(stderr, "[usage] ./obs [-m] <program> [args...]\n");
		return 1;
	}

	bool monitor = false;
	int arg_offset = 1;
	if (strncmp(argv[1], "-m", 2) == 0) {
		monitor = true;
		arg_offset++;
	}

    char temp_profiling_results [] = "profiling.out";
    FILE* fptr = fopen(temp_profiling_results, "w");
    if (fptr == NULL) {
        fprintf(stderr, "Failed to create %s\n", temp_profiling_results);
        return -1;
    }

	struct timespec t0;
	clock_gettime(CLOCK_MONOTONIC, &t0);

	pid_t exec_pid = fork();
	if (exec_pid == 0) {
		int err = execvp(argv[arg_offset], argv + arg_offset);
		if (err < 0) {
			fprintf(stderr, "[error] execv: %s (%d)\n", strerror(errno), errno);
			return -1;
		}
	}

    int wstatus = 0;
    waitpid(exec_pid, &wstatus, 0);
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    struct rusage usage;
    getrusage(RUSAGE_CHILDREN, &usage);

    fprintf(fptr, "[time] (ns) start: %ld, end: %ld, duration: %ld\n",
            t0.tv_sec * 1000 * 1000 * 1000 + t0.tv_nsec,
            t1.tv_sec * 1000 * 1000 * 1000 + t1.tv_nsec,
            (t1.tv_sec - t0.tv_sec) * 1000 * 1000 * 1000 + (t1.tv_nsec - t0.tv_nsec));
    fprintf(fptr, "[time] (us) sys: %ld user: %ld\n",
            usage.ru_stime.tv_sec * 1000 * 1000 + usage.ru_stime.tv_usec,
            usage.ru_utime.tv_sec * 1000 * 1000 + usage.ru_utime.tv_usec);
    fprintf(fptr, "[mem] (kb) maxrss: %ld\n", usage.ru_maxrss);
    fclose(fptr);
	return WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 1;
}
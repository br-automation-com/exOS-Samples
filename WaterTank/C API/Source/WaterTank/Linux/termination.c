#include "termination.h"
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

static bool terminate_process = false;

bool is_terminated()
{
    return terminate_process;
}

static void handle_segfault(int sig) {
	void *array[10];
	size_t size;
	
	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: segfault\n");
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}

static void handle_term_signal(int signum)
{
    switch (signum)
    {
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
        terminate_process = true;
        break;

    default:
        break;
    }
}

void catch_termination()
{
    struct sigaction new_action;

    // Register termination handler for signals with termination semantics
    new_action.sa_handler = handle_term_signal;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;

    // Sent via CTRL-C.
    sigaction(SIGINT, &new_action, NULL);

    // Generic signal used to cause program termination.
    sigaction(SIGTERM, &new_action, NULL);

    // Terminate because of abnormal condition.
    sigaction(SIGQUIT, &new_action, NULL);

    // Print backtrace to stderr and exit() on segfault
	signal(SIGSEGV, handle_segfault); 
}

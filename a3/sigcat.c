/* FILE: sigcat.c
 *
 * AUTHOR: Tariq Soliman
 * STUDENT NO.: 45287316
 *
 * DESCRIPTION: 
 * The entry point for sigcat. sigcat reads one line at a time from stdin, and
 * immediately writes and flushes that line to an output stream. The default
 * output stream is stdout, however the output stream can be changed at runtime
 * between stdout and stderr by sending sigcat SIGSUR 1 and SIGSUR2 signals.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <csse2310a3.h>
#include <signal.h>

#define MIN_SIG 1
#define MAX_SIG 31

// The output stream that can be changed by sending SIGUSR1 or SIGUSR2 signal
FILE* outputStream; // what naming convention should I use?

/* print the signal that was received to the current output stream and update
 * the output steam if SIGUSR1 or SIGUSR2 is received.
 */
void print_signal(int signum) {
    fprintf(outputStream, "sigcat received %s\n", strsignal(signum));
    fflush(outputStream);

    // Change the output stream if a SIGUSR signal is sent.
    if (signum == SIGUSR1) {
        outputStream = stdout;
    } else if (signum == SIGUSR2) {
        outputStream = stderr;
    }
}

/* Set up signal handling for signals sig_num. 
 *
 * Params:
 *      signum: The signal number as defined by the OS to be handled.
 * 
 * Return:
 *      0 if the signal handler was set successfully or -1 if not.
 */
int setup_sig_handling(int signum) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = print_signal;
    return sigaction(signum, &sa, NULL);
}

/* Entry point for sigcat. */
int main(int argc, char* argv[]) {
    outputStream = stdout; // The output stream is stdout by default

    // Set up the signal handling
    for (int signum = MIN_SIG; signum <= MAX_SIG; signum++) {
        setup_sig_handling(signum); // Don't care if it fails for KILL or STOP
    }

    while (1) {
        char* line = read_line(stdin);
        if (!line) { // EOF
            break;
        }

        fprintf(outputStream, "%s\n", line);
        fflush(outputStream);
        free(line);
    }
    return 0;
}

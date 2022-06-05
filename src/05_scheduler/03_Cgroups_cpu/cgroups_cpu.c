#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>

extern int errno;

sig_atomic_t running = 1;

void catch_signal(int sig)
{
	if (sig == SIGINT)
		running = 0;
}

int main() 
{
	// Create new childs process
	pid_t pid1 = fork();
	if (pid1 > 0)
	{
		// Parent code
		while (running == 1);
		
		int wstatus;
		wait(&wstatus);
		printf("exited with %d\n", wstatus);
		
		return EXIT_SUCCESS;
	}
    // Child 
	else
	{
		// Child's code
		while (running == 1);
		exit(0);
	}

	return EXIT_SUCCESS;
}
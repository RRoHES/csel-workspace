/* socket_pair.c */
#define _GNU_SOURCE
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SZ	100

extern int errno;

// Catching all signals here
void catch_signal(int sig)
{
	printf("Signal catched: %d\n", sig);
}

// Child program
void child(int socket) {
    printf("Process child is running on core %d\n",sched_getcpu());

    char buff[BUFFER_SZ];
    printf("Child started\n");
    printf("Please enter your command to parent:\n");

    while (1)
    {

        if (read(socket, buff, BUFFER_SZ) == -1)
        {
            fprintf(stderr, "Error: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        printf("Parent response: %s\n", buff);

        fgets(buff, BUFFER_SZ, stdin);
        write(socket, buff, strlen(buff) + 1);

        // exit command has entered by user
        if (strcmp(buff, "exit\n") == 0)
        {
            close(socket);
            exit(0);
        }
    }
}

// Parent program
void parent(int socket) {
    int sizeBuf;
    char buff[BUFFER_SZ];

    printf("Process parent is running on core %d\n",sched_getcpu());

    // Send to child some info
    write(socket, "Started", strlen("Started") + 1);
    write(socket, "Please enter your command:", strlen("Please enter your command:") + 1);

    while (1)
    {
        sizeBuf = read(socket, buff, BUFFER_SZ);

        printf("Received %.*s from the child\n",sizeBuf-2,buff);

        if (strcmp(buff, "exit\n") == 0)
        {
            close(socket);  
            exit(0);
        }
        else
            write(socket, "Not a valid cmd", strlen("Not a valid cmd") + 1);
    }
}

int main()
{
    int fd[2];
    static const int parentsocket = 0;
    static const int childsocket = 1;
    pid_t pid;
    cpu_set_t set;
    CPU_ZERO(&set);

    // create socket pair
	if (socketpair(AF_UNIX, SOCK_STREAM , 0, fd) == -1)
	{
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	// Add all signals to catch
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = catch_signal;

	for (int ind = SIGHUP; ind < SIGSYS; ++ind)
		sigaction(ind, &act, NULL);

    // Fork the program
    pid = fork();
    if (pid == 0) { /* if fork returned zero, you are the child */
        close(fd[parentsocket]); /* Close the parent file descriptor */
        CPU_SET(1, &set);
        int ret = sched_setaffinity(0, sizeof(set), &set);
        if (ret == -1)
            exit(EXIT_FAILURE);
        child(fd[childsocket]);
    } else { /* ... you are the parent */
        close(fd[childsocket]); /* Close the child file descriptor */
        CPU_SET(0, &set);
        int ret = sched_setaffinity(0, sizeof(set), &set);
        if (ret == -1)
            exit(EXIT_FAILURE);
        parent(fd[parentsocket]);
    }

    exit(0); /* do everything in the parent and child functions */
}

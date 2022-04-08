#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    char* dev = "/dev/mymodule";
    if (argc > 1) dev = argv[1];

    int fd = open(dev, O_RDWR);

    fd_set set;
    FD_ZERO(&set);

    int interrupts = 0;
    while (1) {
        FD_SET(fd, &set);
        int status = select(fd + 1, &set, 0, 0, 0);
        if (status == -1) printf("error while waiting on signal...\n");
        if (FD_ISSET(fd, &set)) {
            interrupts++;
            printf("Event %d occurred\n", interrupts);
        }
        printf("waked-up(%d), status=%d...\n", fd, status);
    }

    close(fd);
    return 0;
}

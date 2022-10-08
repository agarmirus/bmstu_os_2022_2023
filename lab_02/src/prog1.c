#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CHILD_PROCESSES_COUNT 2

#define SLEEPING_TIME 2

int main(void)
{
    int child_pids[CHILD_PROCESSES_COUNT] = {0};

    for (size_t i = 0; i < CHILD_PROCESSES_COUNT; ++i)
    {
        int child_pid = fork();

        if (child_pid == -1)
        {
            printf("Can't fork\n");

            return EXIT_FAILURE;
        }
        else if (child_pid == 0)
        {
            sleep(SLEEPING_TIME);

            int pid = getpid();
            int ppid = getppid();
            int pgid = getpgrp();

            printf("Child process: ID: %d, Parent ID: %d, Group ID: %d\n", pid, ppid, pgid);

            return EXIT_SUCCESS;
        }
        else
            child_pids[i] = child_pid;
    }

    int pid = getpid();
    int pgid = getpgrp();

    printf("Parent process: ID: %d, Group ID: %d, ", pid, pgid);
    printf("Child processes IDs: ");

    for (size_t i = 0; i < len; ++i)
        printf("%d, ", child_pids[i]);

    printf("\n");

    return EXIT_SUCCESS;
}

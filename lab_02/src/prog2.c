#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define CHILD_PROCESSES_COUNT 2

#define SLEEPING_TIME 1

int main(void)
{
    int child_pids[CHILD_PROCESSES_COUNT] = {0};

    for (size_t i = 0; i < CHILD_PROCESSES_COUNT; ++i)
    {
        int new_pid = fork();

        if (new_pid == -1)
        {
            printf("Can't fork\n");

            return EXIT_FAILURE;
        }
        else if (new_pid == 0)
        {
            int pid = getpid();
            int ppid = getppid();
            int pgid = getpgrp();

            printf("Child process: ID: %d, Parent ID: %d, Group ID: %d\n", pid, ppid, pgid);

            return EXIT_SUCCESS;
        }
        else
        {
            int stat_val = 0;
            int child_pid = wait(&stat_val);

            child_pids[i] = child_pid;

            if (WIFEXITED(stat_val))
                printf("Child process (ID %d) finished with code %d\n", child_pid, WEXITSTATUS(stat_val));
            else if (WIFSIGNALED(stat_val))
                printf("Child process (ID %d) finished by signal %d\n", child_pid, WTERMSIG(stat_val));
            else if (WIFSTOPPED(stat_val))
                printf("Child process (ID %d) stopped by signal %d\n", child_pid, WSTOPSIG(stat_val));
        }
    }

    int pid = getpid();
    int pgid = getpgrp();

    printf("Parent process: ID: %d, Group ID: %d, ", pid, pgid);
    printf("Child processes IDs: ");

    for (size_t i = 0; i < len; ++i)
        printf("%d ", child_pids[i]);

    printf("\n");

    return EXIT_SUCCESS;
}

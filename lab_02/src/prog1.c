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
            printf("Аварийное завершение fork\n");

            return EXIT_FAILURE;
        }
        else if (child_pid == 0)
        {
            int pid = getpid();
            int ppid = getppid();
            int pgid = getpgrp();
            
            printf("Дочерний процесс до блокировки: ID: %d, ID предка: %d, ID группы: %d\n", pid, ppid, pgid);

            sleep(SLEEPING_TIME);

            pid = getpid();
            ppid = getppid();
            pgid = getpgrp();

            printf("Дочерний процесс после блокировки: ID: %d, ID предка: %d, ID группы: %d\n", pid, ppid, pgid);

            return EXIT_SUCCESS;
        }
        else
            child_pids[i] = child_pid;
    }

    int pid = getpid();
    int pgid = getpgrp();

    printf("Родительский процесс: ID: %d, Group ID: %d, ", pid, pgid);
    printf("ID дочерних процессов: ");

    for (size_t i = 0; i < CHILD_PROCESSES_COUNT; ++i)
        printf("%d, ", child_pids[i]);

    printf("\n");

    return EXIT_SUCCESS;
}

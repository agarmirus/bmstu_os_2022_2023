#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CHILD_PROCESSES_COUNT 2

#define SLEEP_TIME 1

int main(void)
{
    pid_t child_pids[CHILD_PROCESSES_COUNT];

    for (size_t i = 0; i < CHILD_PROCESSES_COUNT; ++i)
    {
        if ((child_pids[i] = fork()) == -1)
        {
            perror("Аварийное завершение fork\n");

            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            printf("Дочерний процесс до блокировки: ID: %d, ID предка: %d, ID группы: %d\n", getpid(), getppid(), getpgrp());

            sleep(SLEEP_TIME);

            printf("Дочерний процесс после блокировки: ID: %d, ID предка: %d, ID группы: %d\n", getpid(), getppid(), getpgrp());

            return EXIT_SUCCESS;
        }
        else
            printf("Родительский процесс: ID: %d, Group ID: %d, ID дочернего процесса\n", getpid(), getpgrp(), child_pids[i]);
    }

    return EXIT_SUCCESS;
}

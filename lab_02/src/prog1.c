#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CHILD_PROCESSES_COUNT 2

#define SLEEP_TIME 2

int main(void)
{
    int child_pids[CHILD_PROCESSES_COUNT] = {0};

    for (size_t i = 0; i < CHILD_PROCESSES_COUNT; ++i)
    {
        int child_pid = fork();

        if (child_pid == -1)
        {
            printf("Аварийное завершение fork\n");

            exit(1);
        }
        else if (child_pid == 0)
        {
            printf("Дочерний процесс до блокировки: ID: %d, ID предка: %d, ID группы: %d\n", getpid(), getppid(), getpgrp());

            sleep(SLEEP_TIME);

            printf("Дочерний процесс после блокировки: ID: %d, ID предка: %d, ID группы: %d\n", getpid(), getppid(), getpgrp());

            return EXIT_SUCCESS;
        }
        else
        {
            child_pids[i] = child_pid;

            if (i == CHILD_PROCESSES_COUNT - 1)
            {
                printf("Родительский процесс: ID: %d, Group ID: %d, ", getpid(), getpgrp());
                printf("ID дочерних процессов: ");

                for (size_t i = 0; i < CHILD_PROCESSES_COUNT; ++i)
                    printf("%d ", child_pids[i]);

                printf("\n");
            }
        }
    }

    return EXIT_SUCCESS;
}

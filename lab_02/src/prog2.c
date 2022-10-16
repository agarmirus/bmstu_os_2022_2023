#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define CHILD_PROCESSES_COUNT 2

#define SLEEP_TIME 5

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
            printf("Дочерний процесс: ID: %d, ID предка: %d, ID группы: %d\n", getpid(), getppid(), getpgrp());

            sleep(SLEEP_TIME);

            return EXIT_SUCCESS;
        }
        else
        {
            child_pids[i] = child_pid;

            int stat_val = 0;
            
            waitpid(child_pid, &stat_val, 0);

            if (WIFEXITED(stat_val))
                printf("Дочерний процесс (ID %d) завершил свою работу (код завершения: %d)\n", child_pid, WEXITSTATUS(stat_val));
            else if (WIFSIGNALED(stat_val))
                printf("Дочерний процесс (ID %d) завершился: получен сигнал %d\n", child_pid, WTERMSIG(stat_val));
            else if (WIFSTOPPED(stat_val))
                printf("Дочерний процесс (ID %d) остановился: получен сигнал %d\n", child_pid, WSTOPSIG(stat_val));
            
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
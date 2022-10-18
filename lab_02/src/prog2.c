#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define CHILD_PROCESSES_COUNT 2

#define SLEEP_TIME 10

int main(void)
{
    pid_t child_pids[CHILD_PROCESSES_COUNT];

    for (size_t i = 0; i < CHILD_PROCESSES_COUNT; ++i)
    {
        if ((child_pids[i] = fork()) == -1)
        {
            printf("Аварийное завершение fork\n");

            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            printf("Дочерний процесс: ID: %d, ID предка: %d, ID группы: %d\n", getpid(), getppid(), getpgrp());

            sleep(SLEEP_TIME);

            return EXIT_SUCCESS;
        }
        else
            printf("Родительский процесс: ID: %d, Group ID: %d, ID дочернего процесса\n", getpid(), getpgrp(), child_pids[i]);
    }

    for (size_t i = 0; i < CHILD_PROCESSES_COUNT; ++i)
    {
        int stat_val = 0;
        
        if (waitpid(child_pids[i], &stat_val, 0) == -1)
        {
            printf("Аварийное завершение waitpid\n");

            exit(1);
        }

        if (WIFEXITED(stat_val))
            printf("Дочерний процесс (ID %d) завершил свою работу (код завершения: %d)\n", child_pids[i], WEXITSTATUS(stat_val));
        else if (WIFSIGNALED(stat_val))
            printf("Дочерний процесс (ID %d) завершился: получен сигнал %d\n", child_pids[i], WTERMSIG(stat_val));
        else if (WIFSTOPPED(stat_val))
            printf("Дочерний процесс (ID %d) остановился: получен сигнал %d\n", child_pids[i], WSTOPSIG(stat_val));
    }

    return EXIT_SUCCESS;
}

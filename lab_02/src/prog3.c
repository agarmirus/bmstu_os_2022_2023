#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define CHILD_PROCESSES_COUNT 2

int main(void)
{
    int child_pids[CHILD_PROCESSES_COUNT] = {0};

    for (size_t i = 0; i < CHILD_PROCESSES_COUNT; ++i)
    {
        int new_pid = fork();

        if (new_pid == -1)
        {
            printf("Аварийное завершение fork\n");

            return EXIT_FAILURE;
        }
        else if (new_pid == 0)
        {
            printf("Дочерний процесс: ID: %d, ID предка: %d, ID группы: %d\n", getpid(), getppid(), getpgrp());
            
            int rc = 0;

            if (i == 0)
                rc = execl("/home/agarmir/Univer/aa/lab/lab_03/app.exe", "");
            else if (i == 1)
                rc = execl("/home/agarmir/Univer/cprog/lab_03_06_00/main.exe", "");

            if (rc == -1)
            {
                printf("Ошибка вызова execl\n");

                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
        }
        else
        {
            int stat_val = 0;
            int child_pid = wait(&stat_val);

            child_pids[i] = child_pid;

            if (WIFEXITED(stat_val))
                printf("Дочерний процесс (ID %d) завершил свою работу (код завершения: %d)\n", child_pid, WEXITSTATUS(stat_val));
            else if (WIFSIGNALED(stat_val))
                printf("Дочерний процесс (ID %d) завершился неперехватываемым сигналом (номер сигнала: %d)\n", child_pid, WTERMSIG(stat_val));
            else if (WIFSTOPPED(stat_val))
                printf("Дочерний процесс (ID %d) остановился (номер сигнала: %d)\n", child_pid, WSTOPSIG(stat_val));
        }
    }

    printf("Родительский процесс: ID: %d, Group ID: %d, ", getpid(), getpgrp());
    printf("ID дочерних процессов: ");

    for (size_t i = 0; i < CHILD_PROCESSES_COUNT; ++i)
        printf("%d ", child_pids[i]);

    printf("\n");

    return EXIT_SUCCESS;
}

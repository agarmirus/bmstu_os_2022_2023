#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define CHILD_PROCESSES_COUNT 2

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
            printf("Дочерний процесс: ID: %d, ID предка: %d, ID группы: %d\n", getpid(), getppid(), getpgrp());
            
            pid_t rc = 0;

            if (i == 0)
                rc = execl("/home/agarmir/Univer/aa/lab/lab_03/app.exe", "");
            else if (i == 1)
                rc = execl("/home/agarmir/Univer/cprog/lab_03_06_00/main.exe", "");

            if (rc == -1)
            {
                printf("Ошибка вызова execl\n");

                exit(1);
            }

            return EXIT_SUCCESS;
        }
        else
            printf("Родительский процесс: ID: %d, Group ID: %d, ID дочернего процесса: %d\n", getpid(), getpgrp(), child_pids[i]);
    }

    for (size_t i = 0; i < CHILD_PROCESSES_COUNT; ++i)
    {
        int stat_val = 0;
            
        if (waitpid(child_pids[i], &stat_val, 0) == -1)
        {
            perror("Аварийное завершение waitpid\n");

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

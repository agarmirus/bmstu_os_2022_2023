#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define CHILD_PROCESSES_COUNT 2

#define SLEEP_TIME 5

#define MAX_MSG_SIZE 50

int main(void)
{
    int fd[2];

    if (pipe(fd) == -1)
    {
        printf("Аварийное завершение pipe\n");

        exit(1);
    }

    int child_pids[CHILD_PROCESSES_COUNT] = {0};

    char msg[MAX_MSG_SIZE] = {0};

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

            if (i == 0)
                memcpy(msg, "\nabc\n", 6);
            else if (i == 1)
                memcpy(msg, "\njshaflkdhgjshflkajhskja\n", 26);

            close(fd[0]);
            write(fd[1], msg, strlen(msg));

            printf("Сообщение от дочернего процесса (ID %d) отправлено родительскому процессу:\n%s\n", getpid(), msg);

            return EXIT_SUCCESS;
        }
        else
        {
            child_pids[i] = child_pid;

            printf("\nРодительский процесс: ID: %d, Group ID: %d, ID дочернего процесса: %d\n", getpid(), getpgrp(), child_pid);
        }
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

    close(fd[1]);
    read(fd[0], msg, MAX_MSG_SIZE);

    printf("Сообщения, полученные от дочерних процессов:\n%s\n", msg);

    return EXIT_SUCCESS;
}

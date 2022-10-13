#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define CHILD_PROCESSES_COUNT 2

#define BUF_SIZE 750

int main(void)
{
    int child_pids[CHILD_PROCESSES_COUNT] = {0};
    int fd[2];

    pipe(fd);

    char buf[BUF_SIZE];    

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

            char *child_msg = NULL;

            if (i == 0)
                child_msg = "\n№1\nНочь, улица, фонарь, аптека,\n" \
                            "Бессмысленный и тусклый свет.\n" \
                            "Живи еще хоть четверть века -\n" \
                            "Все будет так. Исхода нет.\n\n";
            else if (i == 1)
                child_msg = "\n№2\nЯ УВОЖУ К ОТВЕРЖЕННЫМ СЕЛЕНЬЯМ,\n" \
                            "Я УВОЖУ СКВОЗЬ ВЕКОВЕЧНЫЙ СТОН,\n" \
                            "Я УВОЖУ К ПОГИБШИМ ПОКОЛЕНЬЯМ.\n\n" \
                            "БЫЛ ПРАВДОЮ МОЙ ЗОДЧИЙ ВДОХНОВЛЕН:\n" \
                            "Я ВЫСШЕЙ СИЛОЙ, ПОЛНОТОЙ ВСЕЗНАНЬЯ\n" \
                            "И ПЕРВОЮ ЛЮБОВЬЮ СОТВОРЕН.\n\n" \
                            "ДРЕВНЕЙ МЕНЯ ЛИШЬ ВЕЧНЫЕ СОЗДАНЬЯ,\n" \
                            "И С ВЕЧНОСТЬЮ ПРЕБУДУ НАРАВНЕ.\n" \
                            "ВХОДЯЩИЕ, ОСТАВЬТЕ УПОВАНЬЯ.\n";

            close(fd[0]);
            write(fd[1], child_msg, strlen(child_msg) - 1);

            printf("Сообщение №%d отправлено родительскому процессу:\n%s\n", i + 1, child_msg);

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

                close(fd[1]);
                read(fd[0], buf, BUF_SIZE);

                printf("Сообщения, полученные от дочерних процессов:\n%s\n", buf);
            }
        }
    }

    return EXIT_SUCCESS;
}

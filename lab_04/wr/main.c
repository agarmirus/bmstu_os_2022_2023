#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/wait.h>

/*
 * Использовать функцию ftok
 */

#define CHILDREN_COUNT 7
#define WRITERS_COUNT 3

static int reader_action(const int shmid, const int semsid)
{}

static int writer_action(const int shmid, const int semsid)
{}

int main(void)
{
    /*
     * S_IRWXU - пользователь имеет право на чтение, запись и выполнение файла
     * S_IRWXG - группа имеет право на чтение, запись и выполнение файла
     * S_IRWXO - остальные имеют право на чтение, запись и выполнение файла
     */
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    // Создаем сегмент разделяемой памяти
    int shmid = shmget(100, 1024, O_CREAT | perms);
    if (shmid == -1)
    {
        perror("Cannot create shared memory");
        exit(1);
    }
    // Создаем набор семафоров
    int semsid = semget(100, 1, IPC_CREAT | perms);
    if (semsid == -1)
    {
        perror("Cannot create semaphore set");
        exit(1);
    }
    // Создаем писателей
    pid_t child_pids[CHILDREN_COUNT];
    for (size_t i = 0; i < WRITERS_COUNT; ++i)
    {
        if ((child_pids[i] = fork()) == -1)
        {
            perror("Cannot fork\n");
            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            int rc = reader_action(shmid, semsid);
            if (rc != EXIT_SUCCESS)
            {
                perror("Cannot read from shared memory");
                exit(1);
            }
            return EXIT_SUCCESS;
        }
    }
    // Создаем читателей
    for (size_t i = WRITERS_COUNT; i < CHILDREN_COUNT; ++i)
    {
        if ((child_pids[i] = fork()) == -1)
        {
            perror("Cannot fork\n");
            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            int rc = writer_action(shmid, semsid);
            if (rc != EXIT_SUCCESS)
            {
                perror("Cannot write in shared memory");
                exit(1);
            }
            return EXIT_SUCCESS;
        }
    }
    // Ждем завершения процессов
    for (size_t i = 0; i < CHILDREN_COUNT; ++i)
    {
        int status;
        if (waitpid(child_pids[i], &status, 0) == -1)
        {
            perror("Waitpid error");
            exit(1);
        }

        if (WIFEXITED(status))
            printf("Дочерний процесс (ID %d) завершил свою работу (код завершения: %d)\n", child_pids[i], WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("Дочерний процесс (ID %d) завершился: получен сигнал %d\n", child_pids[i], WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("Дочерний процесс (ID %d) остановился: получен сигнал %d\n", child_pids[i], WSTOPSIG(status));
    }
}

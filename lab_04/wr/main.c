#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/wait.h>

#define READERS_COUNT_SEM 0     // Количество читателей в очереди
#define WRITERS_COUNT_SEM 1     // Количество писателей в очереди
#define IS_READING_SEM 2        // Активный читатель
#define IS_WRITING_SEM 3        // Активный писатель
#define BIN_CR 4                // Захват критической секции

#define CHILDREN_COUNT 7
#define WRITERS_COUNT 3

static int start_read(const int semid)
{
    struct sembuf sops[] = {
        {READERS_COUNT_SEM, 1, 0},
        {WRITERS_COUNT_SEM, 0, 0},
        {IS_WRITING_SEM, 0, 0},
        {IS_READING_SEM, 1, 0}
    };
    return semop(semid, sops, 4);
}

static int stop_read(const int semid)
{
    struct sembuf sops[] = {
        {READERS_COUNT_SEM, -1, 0},
        {IS_READING_SEM, -1, 0}
    };
    return semop(semid, sops, 2);
}

static int start_write(const int semid)
{
    struct sembuf sops[] = {
        {WRITERS_COUNT_SEM, 1, 0},
        {READERS_COUNT_SEM, 0, 0},
        {IS_WRITING_SEM, 0, 0},
        {BIN_CR, -1, 0},
        {IS_WRITING_SEM, 1, 0}
    };
    return semop(semid, sops, 5);
}

static int stop_write(int semid)
{
    struct sembuf sops[] = {
        {WRITERS_COUNT_SEM, -1, 0},
        {IS_WRITING_SEM, -1, 0},
        {BIN_CR, 1, 0}
    };
    return semop(semid, sops, 3);
}

static int run_reader(void *shmptr, const int semid)
{
    if (!shmptr)
    {
        perror("Invalid shared memory pointer");
        exit(1);
    }
    size_t readings_count = 3; 
    for (size_t i = 0; i < readings_count; ++i)
    {
        if (start_read(semid) == -1)
        {
            perror("Cannot start reading");
            exit(1);
        }
        int value = *(int *)shmptr;
        printf("Reader (PID %d) read: %d", getpid(), value);
        if (stop_read(semid) == -1)
        {
            perror("Cannot stop reading");
            exit(1);
        }
        sleep(rand() % 4);
    }
    return EXIT_SUCCESS;
}

static int run_writer(void *shmptr, const int semid)
{
    if (!shmptr)
    {
        perror("Invalid shared memory pointer");
        exit(1);
    }
    size_t writings_count = 3;
    for (size_t i = 0; i < writings_count; ++i)
    {
        if (start_write(semid) == -1)
        {
            perror("Cannot start writing");
            exit(1);
        }
        int new_value = ++(*(int*)shmptr);
        printf("Writer (PID %d) written: %d", getpid(), new_value);
        if (stop_write(semid) == -1)
        {
            perror("Cannot stop writing");
            exit(1);
        }
        sleep(rand() % 4);
    }
    return EXIT_SUCCESS;
}

int main(void)
{
    key_t key = ftok("/dev/null", IPC_PRIVATE);
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    int shmid = shmget(key, sizeof(int), O_CREAT | perms);
    if (shmid == -1)
    {
        perror("Cannot create shared memory");
        exit(1);
    }
    void *shmptr = shmat(shmid, NULL, 0);
    if (shmptr == (void *)-1)
    {
        perror("Cannot get shared memory address");
        exit(1);
    }
    int semid = semget(key, 5, IPC_CREAT | perms);
    if (semid == -1)
    {
        perror("Cannot create semaphores set");
        exit(1);
    }
    semctl(semid, IS_READING_SEM, SETVAL, 0);
    semctl(semid, IS_WRITING_SEM, SETVAL, 0);
    semctl(semid, READERS_COUNT_SEM, SETVAL, 0);
    semctl(semid, WRITERS_COUNT_SEM, SETVAL, 0);
    semctl(semid, BIN_CR, SETVAL, 0);
    pid_t child_pids[CHILDREN_COUNT];
    for (size_t i = 0; i < WRITERS_COUNT; ++i)
    {
        if ((child_pids[i] = fork()) == -1)
        {
            perror("Cannot fork");
            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            int rc = run_reader(shmptr, semid);
            if (rc != EXIT_SUCCESS)
            {
                perror("Reader error");
                exit(1);
            }
            return EXIT_SUCCESS;
        }
    }
    for (size_t i = WRITERS_COUNT; i < CHILDREN_COUNT; ++i)
    {
        if ((child_pids[i] = fork()) == -1)
        {
            perror("Cannot fork");
            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            int rc = run_writer(shmptr, semid);
            if (rc != EXIT_SUCCESS)
            {
                perror("Writer error");
                exit(1);
            }
            return EXIT_SUCCESS;
        }
    }
    for (size_t i = 0; i < CHILDREN_COUNT; ++i)
    {
        int status;
        if (waitpid(child_pids[i], &status, 0) == -1)
        {
            perror("waitpid error");
            exit(1);
        }

        if (WIFEXITED(status))
            printf("Child process (PID %d) finished (code: %d)\n", child_pids[i], WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("Child process (PID %d) finished. Signal received: %d\n", child_pids[i], WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("Child process (PID %d) stopped. Signal received %d\n", child_pids[i], WSTOPSIG(status));
    }
    if (shmdt(shmptr) == -1)
    {
        perror("shmdt error");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl error");
        exit(1);
    }
    if (semctl(semid, 0, IPC_RMID) == -1)
    {
        perror("semctl error");
        exit(1);
    }
    return EXIT_SUCCESS;
}

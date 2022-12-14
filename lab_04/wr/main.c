#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/wait.h>

#define QUEUE_READERS 0         // Количество читателей в очереди
#define QUEUE_WRITERS 1         // Количество писателей в очереди
#define ACTIVE_READER 2         // Активный читатель
#define ACTIVE_WRITER 3         // Активный писатель
#define BIN_WRITER 4            // Захват критической секции писателем

#define READERS_COUNT 4
#define WRITERS_COUNT 3

struct sembuf start_read_sops[] = {
    {QUEUE_READERS, 1, 0},
    {QUEUE_WRITERS, 0, 0},
    {ACTIVE_WRITER, 0, 0},
    {ACTIVE_READER, 1, 0}
};

struct sembuf stop_read_sops[] = {
    {QUEUE_READERS, -1, 0},
    {ACTIVE_READER, -1, 0}
};

struct sembuf start_write_sops[] = {
    {QUEUE_WRITERS, 1, 0},
    {QUEUE_READERS, 0, 0},
    {ACTIVE_WRITER, 0, 0},
    {BIN_WRITER, -1, 0},
    {ACTIVE_WRITER, 1, 0}
};

struct sembuf stop_write_sops[] = {
    {QUEUE_WRITERS, -1, 0},
    {ACTIVE_WRITER, -1, 0},
    {BIN_WRITER, 1, 0}
};

static int run_reader(const int shmid, const int semid)
{
    void *shmptr = shmat(shmid, NULL, 0);
    if (shmptr == (void *)-1)
    {
        perror("Cannot get shared memory address");
        exit(1);
    }
    srand(getpid());
    size_t readings_count = 3; 
    for (size_t i = 0; i < readings_count; ++i)
    {
        sleep(rand() % 4);
        if (semop(semid, start_read_sops, 4) == -1)
        {
            perror("Cannot start reading");
            exit(1);
        }
        int value = *(int *)shmptr;
        printf("Reader (PID %d) read: %d\n", getpid(), value);
        if (semop(semid, stop_read_sops, 2) == -1)
        {
            perror("Cannot stop reading");
            exit(1);
        }
    }
    if (shmdt(shmptr) == -1)
    {
        perror("shmdt error");
        exit(1);
    }
    return EXIT_SUCCESS;
}

static int run_writer(const int shmid, const int semid)
{
    void *shmptr = shmat(shmid, NULL, 0);
    if (shmptr == (void *)-1)
    {
        perror("Cannot get shared memory address");
        exit(1);
    }
    srand(getpid());
    size_t writings_count = 3;
    for (size_t i = 0; i < writings_count; ++i)
    {
        sleep(rand() % 4);
        if (semop(semid, start_write_sops, 5) == -1)
        {
            perror("Cannot start writing");
            exit(1);
        }
        int new_value = ++(*(int*)shmptr);
        printf("Writer (PID %d) wrote: %d\n", getpid(), new_value);
        if (semop(semid, stop_write_sops, 3) == -1)
        {
            perror("Cannot stop writing");
            exit(1);
        }
    }
    if (shmdt(shmptr) == -1)
    {
        perror("shmdt error");
        exit(1);
    }
    return EXIT_SUCCESS;
}

int main(void)
{
    key_t key = ftok("keyfile", IPC_PRIVATE);
    if (key == -1)
    {
        perror("Cannot create key with ftok");
        exit(1);
    }
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    int shmid = shmget(key, sizeof(int), perms | IPC_CREAT);
    if (shmid == -1)
    {
        perror("Cannot create shared memory");
        exit(1);
    }
    int semid = semget(key, 5, perms | IPC_CREAT);
    if (semid == -1)
    {
        perror("Cannot create semaphores set");
        exit(1);
    }
    semctl(semid, ACTIVE_READER, SETVAL, 0);
    semctl(semid, ACTIVE_WRITER, SETVAL, 0);
    semctl(semid, QUEUE_READERS, SETVAL, 0);
    semctl(semid, QUEUE_WRITERS, SETVAL, 0);
    semctl(semid, BIN_WRITER, SETVAL, 1);
    pid_t child_pids[READERS_COUNT + WRITERS_COUNT - 1];
    for (size_t i = 0; i < READERS_COUNT; ++i)
    {
        if ((child_pids[i] = fork()) == -1)
        {
            perror("Cannot fork");
            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            if (run_reader(shmid, semid) != EXIT_SUCCESS)
            {
                perror("Reader error");
                exit(1);
            }
            return EXIT_SUCCESS;
        }
    }
    for (size_t i = READERS_COUNT; i < WRITERS_COUNT + READERS_COUNT - 1; ++i)
    {
        if ((child_pids[i] = fork()) == -1)
        {
            perror("Cannot fork");
            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            if (run_writer(shmid, semid) != EXIT_SUCCESS)
            {
                perror("Writer error");
                exit(1);
            }
            return EXIT_SUCCESS;
        }
    }
    if (run_writer(shmid, semid) != EXIT_SUCCESS)
    {
        perror("Writer error");
        exit(1);
    }
    for (size_t i = 0; i < WRITERS_COUNT + READERS_COUNT - 1; ++i)
    {
        int status;
        if (waitpid(child_pids[i], &status, 0) == -1)
        {
            perror("waitpid error");
            exit(1);
        }
        if (!WIFEXITED(status))
            printf("Child process (PID %d) crashed\n", child_pids[i]);
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/wait.h>

#define EMPTY_CELLS_SEM 0   // Количество пустых ячеек буфера
#define FILED_CELLS_SEM 1   // Количество заполненных ячеек буфера
#define BIN_SEM 2           // Бинарный семафор (чтение из буфера или запись в буфер)

#define CONSUMERS_COUNT 3
#define PRODUCERS_COUNT 3

typedef struct
{
    char *cptr;     // Указатель для потребителя
    char *pptr;     // Указатель для производителя
    char letter;
    char buf[];     // Буфер
} buffer_t;

static int run_producer(const int shmid, const int semid)
{
    buffer_t *shmptr = shmat(shmid, NULL, 0);
    if (shmptr == (void *)-1)
    {
        perror("Cannot get shared memory address");
        exit(1);
    }
    srand(getpid());
    size_t producings_count = 5;
    for (size_t i = 0; i < producings_count; ++i)
    {
        sleep(rand() % 4);
        struct sembuf startsops[] = {
            {EMPTY_CELLS_SEM, -1, 0},
            {BIN_SEM, -1, 0}
        };
        if (semop(semid, startsops, 2) == -1)
        {
            perror("Cannot start producer");
            exit(1);
        }
        *shmptr->pptr = shmptr->letter;
        printf("Producer (PID %d) wrote: %c\n", getpid(), shmptr->letter);
        ++shmptr->letter;
        ++shmptr->pptr;
        struct sembuf stopsops[] = {
            {BIN_SEM, 1, 0},
            {FILED_CELLS_SEM, 1, 0}
        };
        if (semop(semid, stopsops, 2) == -1)
        {
            perror("Cannot stop producer");
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

static int run_consumer(const int shmid, const int semid)
{
    buffer_t *shmptr = shmat(shmid, NULL, 0);
    if (shmptr == (void *)-1)
    {
        perror("Cannot get shared memory address");
        exit(1);
    }
    srand(getpid());
    size_t consumings_count = 5;
    for (size_t i = 0; i < consumings_count; ++i)
    {
        sleep(rand() % 4);
        struct sembuf startsops[] = {
            {FILED_CELLS_SEM, -1, 0},
            {BIN_SEM, -1, 0}
        };
        if (semop(semid, startsops, 2) == -1)
        {
            perror("Cannot start producer");
            exit(1);
        }
        printf("Consumer (PID %d) read: %c\n", getpid(), *shmptr->cptr);
        ++shmptr->cptr;
        struct sembuf stopsops[] = {
            {BIN_SEM, 1, 0},
            {EMPTY_CELLS_SEM, 1, 0}
        };
        if (semop(semid, stopsops, 2) == -1)
        {
            perror("Cannot stop producer");
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
    int shmid = shmget(key, 64, perms | IPC_CREAT);
    if (shmid == -1)
    {
        perror("Cannot create shared memory");
        exit(1);
    }
    buffer_t *shmptr = shmat(shmid, NULL, 0);
    if (shmptr == (void *)-1)
    {
        perror("Cannot get shared memory address");
        exit(1);
    }
    shmptr->cptr = shmptr->pptr = shmptr->buf;
    shmptr->letter = 'a';
    if (shmdt(shmptr) == -1)
    {
        perror("shmdt error");
        exit(1);
    }
    int semid = semget(key, 3, perms | IPC_CREAT);
    if (semid == -1)
    {
        perror("Cannot create semaphores set");
        exit(1);
    }
    semctl(semid, EMPTY_CELLS_SEM, SETVAL, 64);
    semctl(semid, FILED_CELLS_SEM, SETVAL, 0);
    semctl(semid, BIN_SEM, SETVAL, 1);
    pid_t child_pids[PRODUCERS_COUNT + CONSUMERS_COUNT - 1];
    for (size_t i = 0; i < PRODUCERS_COUNT - 1; ++i)
    {
        if ((child_pids[i] = fork()) == -1)
        {
            perror("Cannot fork");
            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            if (run_producer(shmid, semid) != EXIT_SUCCESS)
            {
                perror("Consumer error");
                exit(1);
            }
            return EXIT_SUCCESS;
        }
    }
    for (size_t i = PRODUCERS_COUNT - 1; i < PRODUCERS_COUNT + CONSUMERS_COUNT - 1; ++i)
    {
        if ((child_pids[i] = fork()) == -1)
        {
            perror("Cannot fork");
            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            if (run_consumer(shmid, semid) != EXIT_SUCCESS)
            {
                perror("Producer error");
                exit(1);
            }
            return EXIT_SUCCESS;
        }
    }
    if (run_producer(shmid, semid) != EXIT_SUCCESS)
    {
        perror("Producer error");
        exit(1);
    }
    for (size_t i = 0; i < PRODUCERS_COUNT + CONSUMERS_COUNT - 1; ++i)
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

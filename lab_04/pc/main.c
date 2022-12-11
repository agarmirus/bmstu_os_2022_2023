#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/wait.h>

#define EMPTY_CELLS_SEM 0
#define FILED_CELLS_SEM 1
#define BIN_SEM 2

#define BUFFER_ELEMS_COUNT 15

#define CHILDREN_COUNT 6
#define PRODUCERS_COUNT 3

typedef struct
{
    char *cptr;     // Указатель для потребителя
    char *pptr;     // Указатель для производителя
    char letter;
} buffer_t;

static int start_produce(const int semid)
{
    struct sembuf sops[] = {
        {EMPTY_CELLS_SEM, -1, 0},
        {BIN_SEM, -1, 0}
    };
    return semop(semid, sops, 2);
}

static int stop_produce(const int semid)
{
    struct sembuf sops[] = {
        {BIN_SEM, 1, 0},
        {FILED_CELLS_SEM, 1, 0}
    };
    return semop(semid, sops, 2);
}

static int start_consume(const int semid)
{
    struct sembuf sops[] = {
        {FILED_CELLS_SEM, -1, 0},
        {BIN_SEM, -1, 0}
    };
    return semop(semid, sops, 2);
}

static int stop_consume(int semid)
{
    struct sembuf sops[] = {
        {BIN_SEM, 1, 0},
        {EMPTY_CELLS_SEM, 1, 0}
    };
    return semop(semid, sops, 2);
}

static int run_producer(buffer_t *shmptr, const int semid)
{
    srand(getpid());
    size_t producings_count = 5;
    for (size_t i = 0; i < producings_count; ++i)
    {
        sleep(rand() % 4);
        if (start_produce(semid) == -1)
        {
            perror("Cannot start producer");
            exit(1);
        }
        *shmptr->pptr = shmptr->letter;
        printf("Producer (PID %d) wrote: %c\n", getpid(), shmptr->letter);
        ++shmptr->letter;
        ++shmptr->pptr;
        if (stop_produce(semid) == -1)
        {
            perror("Cannot stop producer");
            exit(1);
        }
    }
    return EXIT_SUCCESS;
}

static int run_consumer(buffer_t *shmptr, const int semid)
{
    srand(getpid());
    size_t consumings_count = 5;
    for (size_t i = 0; i < consumings_count; ++i)
    {
        sleep(rand() % 4);
        if (start_consume(semid) == -1)
        {
            perror("Cannot start producer");
            exit(1);
        }
        printf("Consumer (PID %d) read: %c\n", getpid(), *shmptr->cptr);
        ++shmptr->cptr;
        if (stop_consume(semid) == -1)
        {
            perror("Cannot stop producer");
            exit(1);
        }
    }
    return EXIT_SUCCESS;
}

int main(void)
{
    key_t key = ftok("/dev/null", IPC_PRIVATE);
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    int shmid = shmget(key, BUFFER_ELEMS_COUNT * sizeof(char) + sizeof(buffer_t), perms | IPC_CREAT);
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
    shmptr->cptr = shmptr->pptr = (char *)shmptr + sizeof(buffer_t);
    shmptr->letter = 'a';
    int semid = semget(key, 3, perms | IPC_CREAT);
    if (semid == -1)
    {
        perror("Cannot create semaphores set");
        exit(1);
    }
    semctl(semid, EMPTY_CELLS_SEM, SETVAL, BUFFER_ELEMS_COUNT);
    semctl(semid, FILED_CELLS_SEM, SETVAL, 0);
    semctl(semid, BIN_SEM, SETVAL, 1);
    pid_t child_pids[CHILDREN_COUNT];
    for (size_t i = 0; i < PRODUCERS_COUNT; ++i)
    {
        if ((child_pids[i] = fork()) == -1)
        {
            perror("Cannot fork");
            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            int rc = run_producer(shmptr, semid);
            if (rc != EXIT_SUCCESS)
            {
                perror("Reader error");
                exit(1);
            }
            return EXIT_SUCCESS;
        }
    }
    for (size_t i = PRODUCERS_COUNT; i < CHILDREN_COUNT; ++i)
    {
        if ((child_pids[i] = fork()) == -1)
        {
            perror("Cannot fork");
            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            int rc = run_consumer(shmptr, semid);
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
        if (!WIFEXITED(status))
            printf("Child process (PID %d) crashed\n", child_pids[i]);
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

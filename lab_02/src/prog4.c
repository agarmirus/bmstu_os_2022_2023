#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define CHILD_PROCESSES_COUNT 2

int main(void)
{
    int child_pids[CHILD_PROCESSES_COUNT] = {0};
    int fd[2];

    // ...

    return EXIT_SUCCESS;
}

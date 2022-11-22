#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/wait.h>

/*
 * Никаких массивов не должно быть. Работать только с указателями
 * В инете есть вариант, где задается строка букв латинского алфавита.
 * Достаточно установить значение переменной и инкрементировать ее.
 * Никакого контроля записанных символов делать не надо: и так есть семафоры.
 */

int main(void)
{}

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/resource.h>

#define LOCKFILE "/var/run/mydaemon.pid"

// sigset_t представляет собой набор сигналов
sigset_t mask;

/*
 * Функция для блокировки файла с дескриптором fd для записи
 */
int lockfile(const int fd)
{
    struct flock fl;  // Структура для управления блокировкой файла
    fl.l_type = F_WRLCK;  // Устанавливаем блокировку для записи
    fl.l_whence = SEEK_SET;  // Начало файла
    fl.l_start = 0;  // Устанавливаем смещение 0 относительно l_whence (начало файла)
    fl.l_len = 0;  // Блокировка вплоть до конца файла
    return fcntl(fd, F_SETLK, &fl);
}

int already_running(void)
{
    /*
    * S_IRUSR - доступно пользователю на чтение
    * S_IWUSR - доступно пользователю на запись
    * S_IRGRP - доступно группе на чтение
    * S_IROTH - доступно остальным на чтение
    */
    int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    /*
     * O_RDWR - открыть файл на чтение/запись
     * O_CREAT - создать файл при его отсутствии
     */
    int fd = open(LOCKFILE,  O_RDWR | O_CREAT, perms);
    if (fd == -1)
    {
        syslog(LOG_ERR, "Cannot open %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    if (lockfile(fd) == -1)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return 1;
        }
        syslog(LOG_ERR, "Cannot block %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    /* 
     * Усечение файла необходимо по той причине, что идентификатор предыдущей копии демона,
     * представленный в виде строки, мог иметь большую длину
     */
    ftruncate(fd, 0);
    // Запись идентификатора процесса в файл
    char buf[16];
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);
    return 0;
}

void daemonize(const char *cmd)
{
    // Сброс маски режимов создания файлов
    umask(0);
    // Создаем дочерний процесс и завершаем родительский
    pid_t child_pid;
    if ((child_pid = fork()) == -1)
    {
        syslog(LOG_ERR, "Cannot fork");
        exit(1);
    }
    else if (child_pid > 0)
        exit(0);
    // Обеспечиваем невозможность обретения управляющего терминала в будущем
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) == -1)
    {
        syslog(LOG_ERR, "Cannot ignore SIGHUB signal");
        exit(1);
    }
    // Создаем новую сессию
    if (setsid() == -1)
    {
        syslog(LOG_ERR, "Cannot create new session");
        exit(1);
    }
    // Делаем корневой каталог текущим рабочим каталогом
    if (chdir("/") == -1)
    {
        syslog(LOG_ERR, "Cannot change directory");
        exit(1);
    }
    // Получаем максимальный номер дескриптора файла
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == -1)
    {
        syslog(LOG_ERR, "Cannot get max descriptor number");
        exit(1);
    }
    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    // Закрываем все открытые файлы, в том числе файлы с дескрипторами 0, 1 и 2
    for (int i = 0; i < rl.rlim_max; ++i)
        close(i);
    // Присоединяем файловые дескрипторы 0, 1 и 2 к /dev/null
    int fd0 = open("/dev/null", O_RDWR);
    int fd1 = dup(0);
    int fd2 = dup(0);
    // Инициализируем файл журнала
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    // Проверяем файловые дескрипторы
    if (fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        syslog(LOG_ERR, "Invalid file descriptors: %d %d %d", fd0, fd1, fd2);
        exit(1);
    }
}

// Функция чтения
void reread(void)
{
    syslog(LOG_INFO, "reread function msg");
}

void *thr_fn(void *arg)
{
    int err, signo;
    for (;;)
    {
        // Ожидаем сигналы, указанные в mask
        err = sigwait(&mask, &signo);
        if (err != 0) {
            syslog(LOG_ERR, "sigwait failed");
            exit(1);
        }
        switch (signo)
        {
            case SIGHUP:
                syslog(LOG_INFO, "Re-reading configuration file");
                reread();
                break;
            case SIGTERM:
                syslog(LOG_INFO, "got SIGTERM; exiting");
                exit(0);
            default:
                syslog(LOG_INFO, "unexpected signal %d\n", signo);
        }
    }
    return (void*)0;
}

int main(void)
{
    // Переходим в режим демона
    daemonize("mydaemon");
    // Проверяем, не была ли запушеная другая копия демона
    if (already_running())
    {
        syslog(LOG_ERR, "Daemon is already running");
        exit(1);
    }
    // Восстанавливаем действие по умолчанию для SIGHUP и блокируем все сигналы
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
        syslog(LOG_SYSLOG, "Can't restore SIGHUP default");
    // pthread_sigmask - examine and change mask of blocked signals
    // SIG_BLOCK - набор блокируемых сигналов: объединение текущего набора сигналов и set (в нашем случае - mask)
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0)
        syslog(LOG_ERR, "SIG_BLOCK error");
    // Создаем поток для обработки сигналов
    pthread_t tid;
    pthread_create(&tid, NULL, thr_fn, NULL);
    if (tid == -1)
        syslog(LOG_ERR, "Can't spawn thread for signal handler");
    // sigfillset полностью инициализирует набор set, в котором содержатся все сигналы
    sigfillset(&mask);
    int sleep_time = 5;
    while(1)
    {
        sleep(sleep_time);
        syslog(LOG_INFO, "My daemon is working...");
    }
    return EXIT_SUCCESS;
}

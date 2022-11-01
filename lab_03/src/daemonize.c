#include "daemonize.h"

void daemonize(const char *cmd)
{
    // Сброс маску режимов создания файлов
    umask(0);
    // Создаем дочерний процесс и завершаем родительский
    pid_t child_pid;
    if ((child_pid = fork()) == -1)
    {
        perror("Cannot fork");
        exit(1);
    }
    else if (child_pid > 0)
        exit(0);
    // Создаем новую сессию
    if (setsid() == -1)
    {
        perror("Cannot create new session");
        exit(1);
    }
    // Устанавливаем реакцию процесса на сигнал SIGHUB
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) == -1)
    {
        perror("Cannot ignore SIGHUB signal");
        exit(1);
    }
    // Делаем корневой каталог текущим рабочим каталогом
    if (chdir("/") == -1)
    {
        perror("Cannot change directory");
        exit(1);
    }
    // Получаем максимальный номер дескриптора файла
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == -1)
    {
        perror("Cannot get max descriptor number");
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
        syslog(LOG_ERR, "invalid file descriptors %d %d %d", fd0, fd1, fd2);
        exit(1);
    }
}

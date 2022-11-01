#include "daemonize.h"

int main(void)
{
    daemonize("my_demon");
    while (1)
    {
        sleep(1);
        syslog(LOG_CONS, "abc");
    }
    return EXIT_SUCCESS;
}

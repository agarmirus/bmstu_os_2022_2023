#ifndef DAEMONIZE_H
#define DAEMONIZE_H

#include <stdio.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>

void daemonize(const char *cmd);

#endif

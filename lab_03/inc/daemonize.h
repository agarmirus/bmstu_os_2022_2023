#ifndef DAEMONIZE_H
#define DAEMONIZE_H

#include <sys/resource.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void daemonize(const char *cmd);

#endif

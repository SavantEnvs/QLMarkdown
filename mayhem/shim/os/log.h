// Linux stand-in for Apple's <os/log.h>: logging is compiled out.
#ifndef MAYHEM_SHIM_OS_LOG_H
#define MAYHEM_SHIM_OS_LOG_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

typedef void *os_log_t;

#define os_log_create(subsystem, category) ((os_log_t)0)
#define os_log_error(log, ...) ((void)(log))
#define os_log_info(log, ...) ((void)(log))
#define os_log_debug(log, ...) ((void)(log))

#endif

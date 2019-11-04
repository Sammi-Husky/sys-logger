#ifndef SYSLOGGER_H
#define SYSLOGGER_H

#include <switch.h>
#include <string.h>

#include "useful.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int syslogger_connect(in_addr_t address, uint16_t port) LINKABLE;
extern int syslogger_send(const char* format, ...) LINKABLE;
#ifdef __cplusplus
}
#endif

#endif // SYSLOGGER_H

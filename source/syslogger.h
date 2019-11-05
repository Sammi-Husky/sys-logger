#ifndef SYSLOGGER_H
#define SYSLOGGER_H

#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int syslogger_connect(in_addr_t address, uint16_t port);
extern int syslogger_send(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // SYSLOGGER_H
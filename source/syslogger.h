#ifndef SYSLOGGER_H
#define SYSLOGGER_H

#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int syslogger_init(void);
extern int syslogger_send(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // SYSLOGGER_H

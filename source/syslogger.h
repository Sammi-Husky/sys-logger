#ifndef SYSLOGGER_H
#define SYSLOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

int syslogger_init(void);
int syslogger_send(const char* format, ...);
int syslogger_listen();
#ifdef __cplusplus
}
#endif

#endif // SYSLOGGER_H

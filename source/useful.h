#ifndef USEFUL_H
#define USEFUL_H

#include <string.h>


#define LINKABLE __attribute__ ((weak))
#define sizearray(a)  (sizeof(a) / sizeof((a)[0]))

static inline void debug_log(const char* format, ...)
{
    char buffer[256];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 256, format, args);
    va_end(args);
    
    FILE* f = fopen("sdmc:/syslog/syslog.log", "ab");
    if (f)
    {
        fwrite(buffer, strlen(buffer), 1, f);
        fclose(f);
    }
}
    
#endif // USEFUL_H

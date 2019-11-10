#include <switch.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/errno.h>

#include "useful.h"
#include "syslogger.h"

static int srv_socket = -1;
static struct sockaddr_in srvaddr, cliaddr;

int syslogger_init(void)
{
    srv_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (srv_socket == -1)
    {
        debug_log("Syslogger: Failed to create socket\n");
        close(srv_socket);
        return 1;
    }

    memset(&srvaddr, 0, sizeof(srvaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srvaddr.sin_port = htons(28280);

    if (bind(srv_socket, (const struct sockaddr *)&srvaddr, sizeof(srvaddr)) < 0)
    {
        debug_log("Syslogger: Failed to bind socket\n");
        close(srv_socket);
        return 1;
    }
    return 0;
}

int syslogger_listen()
{
    while (1)
    {
        int n;
        socklen_t len;
        char buffer[256];
        n = recvfrom(srv_socket, (char *)buffer, 256, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';

        if (strncmp(buffer, "Hello Switch", sizeof(buffer)))
        {
            debug_log("Syslogger: Stranger Danger (%s), skipping..\n", inet_ntoa(cliaddr.sin_addr));
            return 1;
        }

        debug_log("Syslogger: Client Connected: %s\n", inet_ntoa(cliaddr.sin_addr));
        if (sendto(srv_socket, "Hello Friend", strlen("Hello Friend"), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0)
        {
            debug_log("Syslogger: Failed to send reply\n");
            return 1;
        }

        return 0;
    }
}

int syslogger_send(const char *format, ...)
{
    if (srv_socket == -1)
    {
        debug_log("Syslogger: Socket not valid\n");
        return 1;
    }
    char buffer[256];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 256, format, args);
    va_end(args);

    if (sendto(srv_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0)
    {
        debug_log("Syslogger: Failed to send message\n");
        return 1;
    }

    return 0;
}
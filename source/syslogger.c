#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/errno.h>

#include "useful.h"
#include "syslogger.h"

static int srv_socket = -1;
static struct sockaddr_in srv_addr, cli_addr;
int syslogger_init(void)
{
    srv_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (srv_socket == -1)
    {
        close(srv_socket);
        return -1;
    }

    bzero(&srv_addr, sizeof srv_addr);
    bzero(&cli_addr, sizeof cli_addr);

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(28280);

    if (bind(srv_socket, (const struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0)
    {
        debug_log("Sockets: Failed to bind\n");
        close(srv_socket);
        return -1;
    }

    int len, n;
    char buffer[256];
    n = recvfrom(srv_socket, (char *)buffer, 256, MSG_WAITALL, (struct sockaddr *)&cli_addr, &len);
    buffer[n] = '\0';

    if(strncmp(buffer, "Hello Switch", sizeof(buffer)))
    {
        debug_log("Sockets: Wrong Client, skipping..");
        close(srv_socket);
        return -1;
    }

    debug_log("Sockets: Client Connected - %s\n", inet_ntoa(cli_addr.sin_addr));

    if(sendto(srv_socket, "Hello From sys-logger!", strlen("Hello From sys-logger!"), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0)
    {
        debug_log("Sockets: Failed to send reply\n");
        close(srv_socket);
        return -1;
    }
    return 0;
}

int syslogger_send(const char *format, ...)
{
    if (srv_socket == -1)
    {
        debug_log("Sockets: Socket not valid\n");
        return -1;
    }
    char buffer[256];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 256, format, args);
    va_end(args);

    if(sendto(srv_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0)
    {
        debug_log("Sockets: Failed to send message\n");
        close(srv_socket);
        return -1;
    }

    return 0;
}
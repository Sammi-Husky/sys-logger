#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/errno.h>

#include "useful.h"
#include "syslogger.h"

static int srv_socket = -1;
static int ipaddress = 0;
static int port = 0;
static struct sockaddr_in srv_addr;
int syslogger_connect(in_addr_t address, uint16_t port)
{
    srv_socket = socket(AF_INET, SOCK_STREAM, 0);
    ipaddress = address;
    if (srv_socket == -1)
    {
        close(srv_socket);
        return -1;
    }

    bzero(&srv_addr, sizeof srv_addr);

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = ipaddress;
    srv_addr.sin_port = port;

    int result = connect(srv_socket, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
    if (result == -1)
    {
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

    if (send(srv_socket, buffer, strlen(buffer), 0) < 0)
    {
        debug_log("Sockets: Error sending data..\n");
        debug_log("Sockets: Reconnecting..\n");
        close(srv_socket);
        bool success = false;

        for (int i = 0; i < 100; i++)
        {
            success = syslogger_connect(ipaddress, port) != -1;
            if (success)
                break;

            svcSleepThread(10000000L);
        }
        if (!success)
        {
            debug_log("Sockets: Connection timed out\n");
            return -1;
        }
        else
        {
            send(srv_socket, buffer, strlen(buffer), 0);
        }
    }

    return 0;
}
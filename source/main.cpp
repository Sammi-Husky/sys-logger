#include <switch.h>

#include <dirent.h>
#include <stdarg.h>
#include <arpa/inet.h>

#include "useful.h"
#include "syslogger.h"
#include "minIni.h"

extern "C"
{
    extern u32 __start__;

    static char g_heap[0x540000];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}

u32 __nx_applet_type = AppletType_None;

char ipaddress[100];
u64 port;

struct LogPacket
{
    bool dirty = false;
    bool to_sd = false;
    char buffer[256];
};

void __libnx_initheap(void)
{
    extern char *fake_heap_start;
    extern char *fake_heap_end;

    fake_heap_start = &g_heap[0];
    fake_heap_end = &g_heap[sizeof g_heap];
}

void __appInit(void)
{
    smInitialize();

    // Wait for SD card to be online
    svcSleepThread(1 * 1000 * 1000 * 1000);
    debug_log("Init: Waiting for SD card..\n");
    for (int i = 0; i < 7; i++)
    {
        svcSleepThread(1 * 1000 * 1000 * 1000);
    }

    // init fs stuff
    fsInitialize();
    fsdevMountSdmc();

    mkdir("sdmc:/syslog", 0700);
    unlink("sdmc:/syslog/syslog.log");
    unlink("sdmc:/SaltySD/syslog.conf");

    debug_log("Init: Reading ini file..\n");
    ini_gets("Network", "pcip", "0.0.0.0", ipaddress, sizearray(ipaddress), "sdmc:/syslog/syslog.ini");
    port = ini_getl("Network", "port", 28280, "sdmc:/syslog/syslog.ini");

    // init sockets
    debug_log("Init: Initializing Socket Driver..\n");
    Result rc = socketInitializeDefault();
    if (R_FAILED(rc))
    {
        debug_log("Init: Socket Driver failed to initialize\n");
    }

    rc = pmdmntInitialize();
    if (R_FAILED(rc))
    {
        fatalSimple(rc);
    }

    // need this to get applicationid
    rc = pminfoInitialize();
    if (R_FAILED(rc))
        fatalSimple(rc);

    debug_log("Init: Trying to set HOS version\n");
    // setting hos version because apparently it changes some functions
    rc = setsysInitialize();
    if (R_SUCCEEDED(rc))
    {
        SetSysFirmwareVersion fw;
        rc = setsysGetFirmwareVersion(&fw);
        if (R_SUCCEEDED(rc))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }
    debug_log("Init: Success!\n");
}

void __appExit(void)
{
    debug_log("Main: Process exiting..\n");
    fsdevUnmountAll();
    fsExit();
    smExit();
    pmdmntExit();
    pminfoExit();
    socketExit();
}

Result waitApplicationLaunch(u64 *pid_out, u64 titleID)
{
    Result rc;
    u64 TID = 0;
    u64 pid = 0;
    while (1)
    {
        while (!pid)
        {
            rc = pmdmntGetApplicationPid(&pid);

            svcSleepThread(10000000L);
        }
        rc = pminfoGetTitleId(&TID, pid);
        if (R_SUCCEEDED(rc) && TID == titleID)
        {
            *pid_out = pid;
            return rc;
        }

        svcSleepThread(10000000L);
    }
}
u64 waitPluginConfig()
{
    u64 addr = 0;
    for (int i=0;i<300;i++)
    {
        char buffer[100];
        FILE *f = fopen("sdmc:/SaltySD/syslog.conf", "r");
        if (f)
        {
            int read_bytes = fread(buffer, 1, 100, f);
            fclose(f);
            buffer[read_bytes] = '\0';
            addr = strtoul(buffer, NULL, 16);
        }
        svcSleepThread(100000000L);
    }
    return addr;
}
int main(int argc, char *argv[])
{
    u64 plugin_log_addr = 0;
    bool connected = false;
    bool pid_invalid = true;
    Handle debug;
    Result rc;
    u64 pid;

    // TODO broadcast UDP packets instead of binding to specific client.
    // This would fix issues with reconnecting as well
    debug_log("Sockets: Attempting to connect to server %s:%d\n", ipaddress, port);
    while (!connected)
    {
        connected = syslogger_init() != -1;
        svcSleepThread(10000000L);
    }
    debug_log("Sockets: Successfully connected!\n");

    // Main loop.
    // Checks if plugin packet is dirty and broadcasts message to client
    LogPacket logger;
    while (appletMainLoop())
    {
        svcSleepThread(10000000L);

        // Wait for smash to start. Also triggered if game was quit or crashed
        if (pid_invalid)
        {
            // Blocks until specified title is launched
            syslogger_send("Waiting for smash to launch..\n");
            waitApplicationLaunch(&pid, 0x01006A800016E000);

            // Wait until plugin gives up it's secrets
            syslogger_send("Waiting for plugin config..\n");
            plugin_log_addr = waitPluginConfig();

            pid_invalid = false;
        }

        rc = svcDebugActiveProcess(&debug, pid);
        if ((pid_invalid = R_FAILED(rc)))
            syslogger_send("Failed to debug process, retcode %llx\n", R_VALUE(rc));

        // only run if smash is running
        if (!pid_invalid)
        {
            // read plugin variable
            rc = svcReadDebugProcessMemory(&logger, debug, (u64)plugin_log_addr, sizeof(logger));
            if ((pid_invalid = R_FAILED(rc)))
                syslogger_send("Failed to read memory, retcode %llx\n", R_VALUE(rc));

            // If the packet is dirty we know a new message is waiting for us.
            // Subsequent calls from plugin will block until we tell it that
            // the message has been sent.
            if (logger.dirty)
            {
                syslogger_send(logger.buffer);
                if (logger.to_sd)
                    debug_log("Plugin: %s", logger.buffer);
                logger.dirty = false;
            }

            // write changes back to plugin
            rc = svcWriteDebugProcessMemory(debug, &logger, plugin_log_addr, sizeof(logger));
            if ((pid_invalid = R_FAILED(rc)))
                syslogger_send("Failed to write memory, retcode %llx\n", R_VALUE(rc));
        }
        // must be closed, even if debugging failed
        svcCloseHandle(debug);
    }

    debug_log("Main: Goodbye\n");
}

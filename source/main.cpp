#include <switch.h>

#include <string>
#include <dirent.h>
#include <stdarg.h>
#include <arpa/inet.h>

#include "useful.h"
#include "syslogger.h"

extern "C"
{
    extern u32 __start__;

    static char g_heap[0x540000];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}

u32 __nx_applet_type = AppletType_None;
void __libnx_initheap(void)
{
    extern char *fake_heap_start;
    extern char *fake_heap_end;

    fake_heap_start = &g_heap[0];
    fake_heap_end = &g_heap[sizeof g_heap];
}

void __appInit(void)
{
    Result rc = smInitialize();
    if (R_FAILED(rc))
        fatalSimple(rc);

    // Wait for SD card to be online
    svcSleepThread(1 * 1000 * 1000 * 1000);
    for (int i = 0; i < 7; i++)
    {
        svcSleepThread(1 * 1000 * 1000 * 1000);
    }

    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalSimple(rc);
    rc = fsdevMountSdmc();
    if (R_FAILED(rc))
        fatalSimple(rc);
    rc = pmdmntInitialize();
    if (R_FAILED(rc))
        fatalSimple(rc);
    rc = pminfoInitialize();
    if (R_FAILED(rc))
        fatalSimple(rc);
    rc = socketInitializeDefault();
    if (R_FAILED(rc))
        fatalSimple(rc);
    rc = setsysInitialize();
    if (R_SUCCEEDED(rc))
    {
        SetSysFirmwareVersion fw;
        rc = setsysGetFirmwareVersion(&fw);
        if (R_SUCCEEDED(rc))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }

    mkdir("sdmc:/syslog", 0700);
    unlink("sdmc:/syslog/syslog.log");
    unlink("sdmc:/SaltySD/syslog.conf");
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
    for (int i = 0; i < 300; i++)
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
    u32 ip;
    debug_log("Sockets: Waiting For Network..\n");
    while (!ip)
    {
        nifmGetCurrentIpAddress(&ip);
        svcSleepThread(10000000L);
    }
    debug_log("Sockets: Connected to network\n");

    if(syslogger_init())
        debug_log("%Syslogger: Failed to init\n");

    if(syslogger_listen())
        debug_log("Syslogger: Couldn't find clients\n");

    u64 plugin_log_addr = 0;
    bool pid_invalid = true;
    Handle debug;
    Result rc;
    u64 pid;

    struct LogPacket
    {
        bool dirty = false;
        bool to_sd = false;
        char buffer[256];
    } logger;

    // Main loop.
    // Checks if plugin packet is dirty and broadcasts message to client
    while (appletMainLoop())
    {
        svcSleepThread(10000000L);

        // Wait for smash to start. Also triggered if game was quit or crashed
        if (pid_invalid)
        {
            // Blocks until specified title is launched
            syslogger_send("Waiting for smash to launch..");
            waitApplicationLaunch(&pid, 0x01006A800016E000);

            // Wait until plugin gives up it's secrets
            syslogger_send("Waiting for plugin config..");
            plugin_log_addr = waitPluginConfig();

            syslogger_send("Success! Plugin log is at %llx", plugin_log_addr);
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
                syslogger_send("Failed to read memory, retcode %llx", R_VALUE(rc));

            // If the packet is dirty we know a new message is waiting for us.
            // Subsequent calls from plugin will block until we tell it that
            // the message has been sent.
            if (logger.dirty)
            {
                syslogger_send(logger.buffer);
                if (logger.to_sd)
                    debug_log("Plugin: %s\n", logger.buffer);
                logger.dirty = false;
            }

            // write changes back to plugin
            rc = svcWriteDebugProcessMemory(debug, &logger, plugin_log_addr, sizeof(logger));
            if ((pid_invalid = R_FAILED(rc)))
                syslogger_send("Failed to write memory, retcode %llx", R_VALUE(rc));
        }
        // must be closed, even if debugging failed
        svcCloseHandle(debug);
    }
    debug_log("Main: Goodbye\n");
}

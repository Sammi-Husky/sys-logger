# sys-logger
Nintendo Switch sysmodule for logging over the LAN from within SaltyNX plugins.

## Installation ##
```
NOTE:
This release includes a basic client to receive messages on, but will misbehave
if a connection is terminated unexpectedly. A better client can easily be made and used.
```

- Copy the contents of sdcard_out to the root of your SD card to install. 

- Edit `sd:/syslog/syslog.ini` with your PC's ip address and the port of your client. The included client port is 28280.

- Included in the releases are header and implementation files for quickly getting your projects started. Simply copy them over and include the `logging.h` header file in your project.

- Currently the sysmodule reads the plugin variable address from a file. This file must be created from your plugin. Place this somewhere in your plugin's `main` function.

```c++
FILE* f = SaltySDCore_fopen("sdmc:/SaltySD/syslog.conf", "w");
if (f) {
  SaltySD_printf("Writing config file...\n");
  char buffer[20];
  snprintf(buffer, 20, "%lx", (u64)&logger);
  SaltySDCore_fwrite(buffer, strlen(buffer), 1, f);
  SaltySDCore_fclose(f);
}
```

## Uninstallation ##
 - By default the title ID used by the sysmodule is 4200000000696969. To remove, just remove this folder from Atmosphere's titles folder. 
 
- If you would just like to temporarily disable the module, rename the flag file in the sysmodule's folder from `boot2.flag` to something else. This will prevent the sysmodule from running on boot.

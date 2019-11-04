# sys-logger
Nintendo Switch sysmodule for logging over the LAN from within SaltyNX plugins.

- Included is also header and implementation files for adding logging support to your projects. Bear in mind, one must still write the address of the variable to a text file in the SaltySD folder in order for the sysmodule to work. Below is an example snippet of saving the address to file. 

## Installation ##
```
NOTE:
This release includes a basic client to receive messages on, but will misbehave if a connection is terminated unexpectedly. A better client can easily be made and used.
```

- Copy the contents of sdcard_out to the root of your SD card to install. 

- Edit `sd:/syslog/syslog.ini` with your PC's ip address and the port of your client.

- Include the `logging.h` header file (provided in releases) in your project and place this somewhere in your plugin's `main` function to get started.

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

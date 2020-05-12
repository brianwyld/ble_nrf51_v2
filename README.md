# Description

This project is made to compile and use the BLE iBeacon + scanner Wyres firmware using GCC toolchain.
It uses the Nordic SDK/SoftDevice system:
 - SDK 12.3.0 (approx)
 - Softdevice S130 v2.0.1 (probably)
[approx because nordic don't include a version header file, so its tricky to tell exactly which version of the SDK/SD you have..]

# Use

All the source files, header files and linker scripts are located in the project.
GCC tools should be installed on the PC, and have their bin in the PATH.
OpenOCD can be used to flash the nrf51, installed in c:/soft/openocd-0.10.0 (otherwise go change flash_w_stlink.bat)

## makefile

You have to set your gcc path in makefile IFF it is not in your PATH eg:
GNU_INSTALL_ROOT := C:\Program Files (x86)\GNU Tools ARM Embedded\8 2019-q3-update 

Run "make cleanOutputs" to clean all previous outputs.
Run "make itall" to build everything for production (no debug)
Run "make itall_debug" to build everything for debug

## flashing with ST-LINK

Attach ST-Link to the target and run "flash_w_stlink.bat" (in script) with arguements: "ble_modem","major_in_hex", minor_in_hex"

Example : for flashing ble_modem.hex for the preidentified target 0x0002002e type :
```
flash_w_stlink.bat ble_modem 0002 002e
```

## debugging with st-link

The debug process on vscode is based on launch.json settings file and uses just openocd (no gdb)
Have a look on launch.json to proper use it (openocd path, executable, etc).

After that you can launch debugging as usual on vscode.

## debugging with j-link

The debug process on vscode is based on launch.json settings file.
Have a look on launch.json to proper use it (openocd path, executable, etc).

If your target has to be powered by jlink, be sure to entering "power on" or "power on perm" with jlink.exe before.  

```
jlink.exe > power on
jlink.exe > exit
```

After that you can launch debugging as usual on vscode.


# Warning

#Known issues

## RAM alignment

nRF51822-QFAA : 	Code size : 256 kB, Page size : 1024 byte, Nb pages : 256, 
					RAM size : 16kB, Block 0 = 8 kB, Block 1 = 8 kB 

As described in softdevice doc : S130_SDS_v2.0.pdf : 15.1 Memory resource map and usage	(see in this repo at /documentation)

With softdevice enabled in your project, you must be stricly aligned with SoftDevice RAM :

```
APP_RAM_BASE address(minimum required value) = 0x20000000 + SoftDevice RAM consumption (Minimum: 0x200013C8)
```
Central and Peripheral operation consume a lot of ram : see components/softdevice/common/softdevice_handle/app_ram_base.h

## User defined symbols and variables

For this project the heap memory allocation is not necessary and may be set to zero.
Heap memory is configured in gcc_nrf51_startup.s according to user definition.
The problem is located when the assembler arm-none-eabi-as is called with its arguments : 
```
-defsym __HEAP_SIZE=16
```
seems to be ignored...
To fix this : gcc_nrf51_startup.s has been modified to set zero for heap size by default.

```
#if defined(__STARTUP_CONFIG)
    .equ Heap_Size, __STARTUP_CONFIG_HEAP_SIZE
#elif defined(__HEAP_SIZE)
    .equ Heap_Size, __HEAP_SIZE
#else
    .equ    Heap_Size, 0
#endif
```

## Static Memory consumption 

After a build success, a brief look at .map allows to know the amount of RAM used by any function :
For example : 
```
.bss COMMON         0x2000396c      0x168 .obj/c/./src/ibs_scan.o
```
shows memory consumption of ibs_scan.c. (due to declaration of ibs_scan_result_t ibs_scan_results[IBS_SCAN_LIST_LENGTH]; )

Note that this project uses baselibc project from github to reduce RAM and flash consumption!

Further investigation are needed to reduce the global RAM memory consumption to avoid this kind of returned error :
```
arm-none-eabi/bin/ld.exe: region RAM overflowed with stack collect2.exe: error: ld returned 1 exit status
```

Currently 1 Central+1 Periph builds with a scan list of 100 items.















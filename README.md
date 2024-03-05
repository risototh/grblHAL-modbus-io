## grblHAL plugin for MODBUS I/O

This plugin enables control of various extension boards to be connecter to grblHAL board via MODBUS serial bus.
It's not perfect, maybe it has some bugs (I'm pretty sure that it has...), but it's a good starting point for simple extension of I/Os of the board.

My intention was to mount small I/O board directly on top of the ATC capable spindle to control the solenoids and reading the sensors directly from the spindle, without the need of another bulky cable. The bonus is, that this is less prone to interference, that analog signals.

This plugin is written for the Teensy 4.1 based open source board made by Phil Barrett: https://github.com/phil-barrett/grblHAL-teensy-4.x

Plugin was tested with this board: https://www.aliexpress.com/item/1005004933766085.html

> Note: The extension board must be set up to the same baudrate and async setting as the grblHAL board. Usually something like 19200 8N1, or 38400 8N1. This can be achieved by setting the `MODBUS_BAUDRATE` constant in _my_machine.h_. 

### HOW TO INSTALL

1) Make a new src directory called `mbio` in the grblHAL _src_ directory and put the content of this repository in the `mbio` directory.
2) Edit _grbl/errors.h_ to add new error code into the `status_code_t` enum `Status_GCodeTimeout`. Add this line under `Status_GCodeCoordSystemLocked = 56,`:
```
    Status_GCodeTimeout = 57,
```
3) Edit _grbl/plugins_init.h_ to add the init code of the plugin. You can add it at the end.
```
	#if MBIO_ENABLE
	    extern void mbio_init (void);
	    mbio_init();
	#endif     
```
4) To enable the plugin, edit _my_machine.h_ and add:
```
    #define MODBUS_ENABLE 1
    #define MBIO_ENABLE 1
```
5) If you want to enable debug messages of the plugin, add this line to _my_machine.h_:
```	
	#define MBIO_DEBUG
```
6) Compile and flash your machine and enjoy.

### HOW TO USE

There are two M-codes implemented for now. `M101` and `M102`.

Format of **M101** is: `M101 D{0..247} E{1,2,3,4,5,6} P{1..9999} [Q{0..65535}]`
- D{0..247} - device address
- E{2,3,4,5,6} - function code, see https://ipc2u.com/articles/knowledge-base/modbus-rtu-made-simple-with-detailed-descriptions-and-examples/#cmnd
- P{1..9999} - register address
- Q{0..65535} - register value, optional, required for function codes {1,5,6}

**Examples:**
- turn on DO1 on slave with address 2: `M101 D2 E5 P1 Q1`
- turn off DO1 on slave with address 2: `M101 D2 E5 P1 Q0`
- read DI2 on slave with address 2: `M101 D2 E2 P2`
- read DO1-DO4 on slave with address 2: `M101 D2 E1 P1 Q4`
- read holding register 254 on slave with address 2: `M101 D2 E3 P254`
- read AI3 on slave with address 2: `M101 D2 E4 P3`

The read values are stored in _sys.var5399_ for use in the ATC macro, but not tested so far.

Format of **M102** is: `M102 D{0..247} P{1..9999} Q{0,1} R{0.0 .. 3600.0}`
- D{0..247} - device address
- P{1..9999} - register address
- Q{0,1} - register value to wait for
- R{0.0 .. 3600.0} - timeout in seconds (it will be more, as the MODBUS communication is currently not counted in)


**Examples**
- read DI2 on slave with address 2, wait for 1 up to 10 seconds: `M102 D2 P2 Q1 R10`
- read DI6 on slave with address 10, wait for 0 up to 5.4 seconds: `M102 D10 P6 Q0 R5.4`

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

## 📘 How to Use

This firmware implements **two M-codes** for Modbus communication: `M101` and `M102`.

---

### 🔹 M101 – Send a Modbus Command

```
M101 D{0..247} E{1,2,3,4,5,6} P{1..9999} [Q{0..65535}]
```

**Parameters**
- `D` – **Device address** of the Modbus slave (0–247)
- `E` – **Function code**
  - `1` – Read Coils
  - `2` – Read Discrete Inputs
  - `3` – Read Holding Registers
  - `4` – Read Input Registers
  - `5` – Write Single Coil
  - `6` – Write Single Register
- `P` – **Register/coil address** (1–9999)  
  *(P=1 corresponds to Modbus address 0 internally)*
- `Q` – **Value** (optional for reads, required for writes)  
  - For **Write Coil** (E=5): `Q1` = ON (0xFF00), `Q0` = OFF (0x0000)
  - For **Write Register** (E=6): `Q` sets register value

**Examples**
```
M101 D2 E5 P1 Q1      ; turn ON DO1 on slave address 2
M101 D2 E5 P1 Q0      ; turn OFF DO1 on slave address 2
M101 D2 E2 P2         ; read DI2 on slave address 2
M101 D2 E1 P1 Q4      ; read DO1–DO4 (4 coils starting at DO1)
M101 D2 E3 P254       ; read Holding Register 254
M101 D2 E4 P3         ; read Analog Input 3 (Input Register 3)
```

**Result:**  
- Read values are stored in `sys.var5399` for later use (e.g., in ATC macros).

---

### 🔹 M102 – Wait for a Value

```
M102 D{0..247} P{1..9999} Q{0,1} R{0.0..3600.0}
```

**Parameters**
- `D` – Device address (0–247)
- `P` – Register/coil address (1–9999)
- `Q` – Desired value (0 or 1)
- `R` – Timeout in seconds (up to 3600)

**Examples**
```
M102 D2 P2 Q1 R10      ; wait up to 10s for DI2 to become 1
M102 D10 P6 Q0 R5.4    ; wait up to 5.4s for DI6 to become 0
```



###Changelog:###
2025-07-29 PvdW Fixed compile errors, tested with grblHAL 20250724

// Enable by #define MBIO_ENABLE 1 in my_machine.h.
// Enable debug by #define MBIO_DEBUG in my_machine.h.

// Short description how to use it:

// M101 D{0..247} E{1,2,3,4,5,6} P{1..9999} [Q{0..65535}]
// - D{0..247} - device address
// - E{2,3,4,5,6} - function code, see https://ipc2u.com/articles/knowledge-base/modbus-rtu-made-simple-with-detailed-descriptions-and-examples/#cmnd
// - P{1..9999} - register address; 
// - Q{0..65535} - register value, optional, required for function codes {1,5,6}

// turn on DO1 on slave with address 2: M101 D2 E5 P1 Q1
// turn off DO1 on slave with address 2: M101 D2 E5 P1 Q0
// read DI2 on slave with address 2: M101 D2 E2 P2 Q1
// read DO1-DO4 on slave with address 2: M101 D2 E1 P1 Q4
// read holding register 254 on slave with address 2: M101 D2 E3 P254
// read AI3 on slave with address 2: M101 D2 E4 P3 

// the read values are stored in sys.var5399 for use in the ATC macro, but not tested

// so far tested only with https://www.aliexpress.com/item/1005004933766085.html


#include "driver.h"

#if MBIO_ENABLE

#include "modbus_io.h"
#include "grbl/config.h"
#include "grbl/hal.h"
#include "grbl/gcode.h"
#include "grbl/modbus.h"
#include "grbl/protocol.h"
#include "grbl/state_machine.h"
#include "grbl/report.h"
#ifdef MBIO_DEBUG
    #include <stdio.h>
#endif

static user_mcode_ptrs_t user_mcode;
static on_report_options_ptr on_report_options;
static void mbio_rx_packet (modbus_message_t *msg);
static void mbio_rx_exception (uint8_t code, void *context);

static const modbus_callbacks_t callbacks = {
    .on_rx_packet = mbio_rx_packet,
    .on_rx_exception = mbio_rx_exception
};

static void mbio_raise_alarm (void *data) {
    system_raise_alarm(Status_ExpressionInvalidResult); // TODO implement own error code?
}

bool mbio_failed(void) {
    bool ok = true;

    if (sys.cold_start) {
        protocol_enqueue_foreground_task(mbio_raise_alarm, NULL);
    }
    else {
        system_raise_alarm(Status_ExpressionInvalidResult); // TODO implement own error code?
        protocol_enqueue_foreground_task(report_warning, "What's this?");
    }

    return ok;
}

static void mbio_rx_exception(uint8_t code, void *context) {
    // Alarm needs to be raised directly to correctly handle an error during reset (the rt command queue is
    // emptied on a warm reset). Exception is during cold start, where alarms need to be queued.
    mbio_failed();
}


static void mbio_report_options(bool newopt) {
    on_report_options(newopt);

    if (newopt) {
        hal.stream.write(",MBIO");
    }
    else {
        hal.stream.write("[PLUGIN:MODBUS IO v0.1]" ASCII_EOL);
    }
}

void mbio_modbus_send_command(modbus_message_t _cmd, bool block) {
#ifdef MBIO_DEBUG
    char buf[30];
    sprintf(buf, "MODBUS TX: %02X %02X %02X %02X %02X %02X", _cmd.adu[0], _cmd.adu[1], _cmd.adu[2], _cmd.adu[3], _cmd.adu[4], _cmd.adu[5]);
    report_message(buf, Message_Plain);
#endif

    modbus_send(&_cmd, &callbacks, true);
}

void mbio_ModBus_ReadCoils(char device_address, uint16_t register_address, uint16_t value) {
    modbus_message_t _cmd = {
        .context = (void *)MBIO_Command,
        .crc_check = true,
        .adu[0] = device_address, // slave device address
        .adu[1] = ModBus_ReadCoils, // function
        .adu[2] = MODBUS_SET_MSB16(register_address), // register address
        .adu[3] = MODBUS_SET_LSB16(register_address),
        .adu[4] = MODBUS_SET_MSB16(value), // value
        .adu[5] = MODBUS_SET_LSB16(value),
        .tx_length = 8,
        .rx_length = 6
    };
    mbio_modbus_send_command(_cmd, true);
}

void mbio_ModBus_WriteCoil(char device_address, uint16_t register_address, uint16_t value) {
    modbus_message_t _cmd = {
        .context = (void *)MBIO_Command,
        .crc_check = true,
        .adu[0] = device_address, // slave device address
        .adu[1] = ModBus_WriteCoil, // function
        .adu[2] = MODBUS_SET_MSB16(register_address), // register address
        .adu[3] = MODBUS_SET_LSB16(register_address),
        .adu[4] = MODBUS_SET_MSB16(value), // value
        .adu[5] = MODBUS_SET_LSB16(value),
        .tx_length = 8,
        .rx_length = 8
    };
    mbio_modbus_send_command(_cmd, true);
}

void mbio_ModBus_ReadDiscreteInputs(char device_address, uint16_t register_address, uint16_t value) {
    modbus_message_t _cmd = {
        .context = (void *)MBIO_Command,
        .crc_check = true,
        .adu[0] = device_address, // slave device address
        .adu[1] = ModBus_ReadDiscreteInputs, // function
        .adu[2] = MODBUS_SET_MSB16(register_address), // register address
        .adu[3] = MODBUS_SET_LSB16(register_address),
        .adu[4] = MODBUS_SET_MSB16(value), // number of registers to read
        .adu[5] = MODBUS_SET_LSB16(value),
        .tx_length = 8,
        .rx_length = 6
    };
    mbio_modbus_send_command(_cmd, true);
}

void mbio_ModBus_ReadHoldingRegisters(char device_address, uint16_t register_address) {
    modbus_message_t _cmd = {
        .context = (void *)MBIO_Command,
        .crc_check = true,
        .adu[0] = device_address, // slave device address
        .adu[1] = ModBus_ReadHoldingRegisters, // function
        .adu[2] = MODBUS_SET_MSB16(register_address), // register address
        .adu[3] = MODBUS_SET_LSB16(register_address),
        .adu[4] = 0x00, // number of registers to read
        .adu[5] = 0x01,
        .tx_length = 8,
        .rx_length = 7
    };
    mbio_modbus_send_command(_cmd, true);
}

void mbio_ModBus_ReadInputRegisters(char device_address, uint16_t register_address, uint16_t value) {
    modbus_message_t _cmd = {
        .context = (void *)MBIO_Command,
        .crc_check = true,
        .adu[0] = device_address, // slave device address
        .adu[1] = ModBus_ReadInputRegisters, // function
        .adu[2] = MODBUS_SET_MSB16(register_address), // register address
        .adu[3] = MODBUS_SET_LSB16(register_address),
        .adu[4] = MODBUS_SET_MSB16(value), // number of registers to read
        .adu[5] = MODBUS_SET_LSB16(value),
        .tx_length = 8,
        .rx_length = 7
    };
    mbio_modbus_send_command(_cmd, true);
}

void mbio_ModBus_WriteRegister(char device_address, uint16_t register_address, uint16_t value) {
    modbus_message_t _cmd = {
        .context = (void *)MBIO_Command,
        .crc_check = true,
        .adu[0] = device_address, // slave device address
        .adu[1] = ModBus_WriteRegister, // function
        .adu[2] = MODBUS_SET_MSB16(register_address), // register address
        .adu[3] = MODBUS_SET_LSB16(register_address),
        .adu[4] = MODBUS_SET_MSB16(value), // number of registers to read
        .adu[5] = MODBUS_SET_LSB16(value),
        .tx_length = 8,
        .rx_length = 8
    };
    mbio_modbus_send_command(_cmd, true);
}


// Check if M-code is handled here.
// parameters: mcode - M-code to check for (some are predefined in user_mcode_t in grbl/gcode.h), use a cast if not.
// returns:    mcode if handled, UserMCode_Ignore otherwise (UserMCode_Ignore is defined in grbl/gcode.h).
static user_mcode_t mbio_check(user_mcode_t mcode) {
    return mcode == UserMCode_Generic1
                     ? mcode
                     : (user_mcode.check ? user_mcode.check(mcode) : UserMCode_Ignore);
}

// Validate M-code parameters: 
// - D[0..247] - device address
// - E[2,3,4,5,6] - function code; 
// - P[1..9999] - register address; 
// - Q[0..65535] - register value
// parameters: gc_block - pointer to parser_block_t struct (defined in grbl/gcode.h).
//             deprecated - 
// returns:    status_code_t enum (defined in grbl/gcode.h): Status_OK if validated ok, appropriate status from enum if not.
static status_code_t mbio_validate(parser_block_t *gc_block, parameter_words_t *deprecated) {
	status_code_t state = Status_GcodeValueWordMissing;

    switch (gc_block->user_mcode) {

        case UserMCode_Generic1:
            // device address D[0..247]: required
            if (!gc_block->words.d || !isintf(gc_block->values.d)) { // Check if D parameter value is supplied.
                state = Status_BadNumberFormat;
            }

            // function code E[2,4,5,6]: required
            if (!gc_block->words.e || !isintf(gc_block->values.e)) { // Check if E parameter value is supplied.
                state = Status_BadNumberFormat;
            }

            // register address P[1..9999]: required
            if (!gc_block->words.p || !isintf(gc_block->values.p)) {// Check if P parameter value is supplied.
                state = Status_BadNumberFormat;
            }

            // value Q[0..65535]: optional, some functions (2,4) have fixed value
            // add check for required Q on certain E!
            if (gc_block->words.q && !isintf(gc_block->values.q)) { // Check if Q parameter value is supplied.
                state = Status_BadNumberFormat;
            }

            // value
            if (state != Status_BadNumberFormat) { // Are required parameters provided?
                // briefly check ranges
                if (gc_block->values.d < 0.0f || gc_block->values.d > 247.0f
                    ||
                    (gc_block->values.e != (float)ModBus_ReadDiscreteInputs && gc_block->values.e != (float)ModBus_ReadInputRegisters 
                        && gc_block->values.e != (float)ModBus_WriteCoil && gc_block->values.e != (float)ModBus_WriteRegister
                        && gc_block->values.e != (float)ModBus_ReadHoldingRegisters && gc_block->values.e != (float)ModBus_ReadCoils)
                    ||
                    gc_block->values.p < 1.0f || gc_block->values.d > 9999.0f
                    ||
                    gc_block->values.q < 0.0f || gc_block->values.d > 65535.0f) {
                	
                    state = Status_GcodeValueOutOfRange;                    
                }
                else {
                    switch ((char)gc_block->values.e) {
                        case ModBus_ReadDiscreteInputs:
                        case ModBus_ReadInputRegisters:
                            gc_block->values.q = 1.0f;    
                            break;
                        case ModBus_WriteCoil:
                            break;
                        case ModBus_WriteRegister:
                            break;
                    }
                    
                	state = Status_OK;
                }
                    
                gc_block->words.d = gc_block->words.e = gc_block->words.p = gc_block->words.q = Off; // Claim parameters.
                //gc_block->user_mcode_sync = true;                           // Optional: execute command synchronized
            }
            break;

        default:
            state = Status_Unhandled;
            break;
    }

    // If not handled by us and another handler present then call it.
    return state == Status_Unhandled && user_mcode.validate ? user_mcode.validate(gc_block, deprecated) : state;
}


// Execute M-code
// parameters: state - sys.state (bitmap, defined in system.h)
//             gc_block - pointer to parser_block_t struct (defined in grbl/gcode.h).
// returns:    -
static void mbio_execute(sys_state_t state, parser_block_t *gc_block) {
    bool handled = true;

    switch(gc_block->user_mcode) {
        case UserMCode_Generic1:
            char device_address = (char)gc_block->values.d;
            uint16_t register_address = (uint16_t)gc_block->values.p - 1;

            uint16_t value = (uint16_t)gc_block->values.q;
            if ((char)gc_block->values.e == 5) {
                value = value > 0 ? 0xff00 : 0;
            }

            switch ((char)gc_block->values.e) {
                case ModBus_ReadCoils: // 1
                    mbio_ModBus_ReadCoils(device_address, register_address, value);
                    break;

                case ModBus_ReadDiscreteInputs: // 2
                    mbio_ModBus_ReadDiscreteInputs(device_address, register_address, 1);
                    break;

                case ModBus_ReadInputRegisters: // 4
                    mbio_ModBus_ReadInputRegisters(device_address, register_address, 1);
                    break;

                case ModBus_ReadHoldingRegisters: // 3
                    mbio_ModBus_ReadHoldingRegisters(device_address, register_address);
                    break;

                case ModBus_WriteCoil: // 5
                    mbio_ModBus_WriteCoil(device_address, register_address, value);
                    break;

                case ModBus_WriteRegister: // 6
                    mbio_ModBus_WriteRegister(device_address, register_address, value);
                    break;
            }
            break;

        default:
            handled = false;
            break;
    }

    // If not handled by us and another handler present, call it.
    if (!handled && user_mcode.execute) {
        user_mcode.execute(state, gc_block);
    }
}

static void mbio_rx_packet (modbus_message_t *msg) {
    if (!(msg->adu[0] & 0x80)) {
        switch((mbio_response_t)msg->context) {
            case MBIO_Command:
                // rewrite in opposite way - use context to distinguish between commands and then check if the response corresponds to command sent!

                // process the responses (put red value into the system.var5399)
                switch (msg->adu[1]) {
                    case ModBus_ReadDiscreteInputs:
                        if (msg->adu[3] == 0x01) {
                            sys.var5399 = 1;
                        }
                        else if (msg->adu[3] == 0x00) {
                            sys.var5399 = 0;
                        }
                        else {
                            mbio_failed(); // not sure, but other values are not expected
                        }
                        break;

                    case ModBus_ReadCoils:
                        sys.var5399 = (int32_t)msg->adu[3];
                        break;

                    case ModBus_ReadInputRegisters:
                    case ModBus_ReadHoldingRegisters:
                        sys.var5399 = (int32_t)modbus_read_u16(&msg->adu[3]);
                        break;
                }

#ifdef MBIO_DEBUG
                char buf[30];
                sprintf(buf, "MODBUS RX: %02X %02X %02X %02X %02X %02X %02X %02X", msg->adu[0], msg->adu[1], msg->adu[2], msg->adu[3], msg->adu[4], msg->adu[5], msg->adu[6], msg->adu[7]);
                report_message(buf, Message_Plain);

                switch (msg->adu[1]) {
                    case ModBus_ReadDiscreteInputs:
                        if (msg->adu[3] == 0x01) {
                            report_message("MODBUS RESPONSE: on", Message_Plain);
                        }
                        else if (msg->adu[3] == 0x00) {
                            report_message("MODBUS RESPONSE: off", Message_Plain);
                        }
                        break;

                    case ModBus_ReadCoils:
                        sprintf(buf, "MODBUS RESPONSE: %d (0x%02X)", msg->adu[3], msg->adu[3]);
                        report_message(buf, Message_Plain);
                        break;

                    case ModBus_ReadInputRegisters:
                    case ModBus_ReadHoldingRegisters:
                        uint16_t value = modbus_read_u16(&msg->adu[3]);
                        sprintf(buf, "MODBUS RESPONSE: %u (0x%04X)", value, value);
                        report_message(buf, Message_Plain);
                        break;

                    case ModBus_WriteCoil:
                        report_message("MODBUS RESPONSE: OK", Message_Plain);
                        break;

                    case ModBus_WriteRegister:
                        report_message("MODBUS RESPONSE: OK", Message_Plain);
                        break;
                }
#endif
                /*
                spindle_data.rpm = (float)((msg->adu[4] << 8) | msg->adu[5]) * rpm_at_50Hz / 5000.0f;
                vfd_state.at_speed = settings.spindle.at_speed_tolerance <= 0.0f || (spindle_data.rpm >= spindle_data.rpm_low_limit && spindle_data.rpm <= spindle_data.rpm_high_limit);
                */
                break;

            default:
                break;
        }
        
    }
    else {
        report_message("MODBUS ERROR", Message_Warning);
    }
}



void mbio_init(void) {
	hal.user_mcode.check = mbio_check;
    hal.user_mcode.validate = mbio_validate;
    hal.user_mcode.execute = mbio_execute;

	on_report_options = grbl.on_report_options;
    grbl.on_report_options = mbio_report_options;
}

#endif
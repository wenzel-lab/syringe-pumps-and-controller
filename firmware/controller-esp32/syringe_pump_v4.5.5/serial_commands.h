#ifndef SERIAL_COMMANDS_H
#define SERIAL_COMMANDS_H

#include <Arduino.h>
#include "config.h"

// Version check
#ifndef FIRMWARE_VERSION
#error "FIRMWARE_VERSION must be defined in config.h"
#endif

// Function declarations
void handleSerialCommands();
void processSerialCommand(const String& cmd);
String getValue(String data, char separator, int index);

// Response messages
#define RESPONSE_OK "OK"
#define RESPONSE_ERROR_CMD "ERROR_CMD"
#define RESPONSE_ERROR_CMD_MISSING_PUMP "ERROR_CMD_MISSING_PUMP"
#define RESPONSE_ERROR_CMD_INVALID_PUMP "ERROR_CMD_INVALID_PUMP"

// External declarations for motor control functions
extern void saveMotorPreset(int index);
extern void printScreen();

#endif // SERIAL_COMMANDS_H
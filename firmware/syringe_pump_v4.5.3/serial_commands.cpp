#include "serial_commands.h"
#include "config.h"
#include <Arduino.h>

// Version check
#ifndef FIRMWARE_VERSION
#error "FIRMWARE_VERSION must be defined in config.h"
#endif

// External declarations for motor control
extern Motor motors[NUM_MOTORS];
extern volatile bool key_change;
extern char actual_motor;
extern void calculateNewStep();
extern void saveMotorPreset(int index);
extern void printScreen();

extern ContinuousStepper<StepperDriver> stepper_a;
extern ContinuousStepper<StepperDriver> stepper_b;
extern ContinuousStepper<StepperDriver> stepper_c;
extern ContinuousStepper<StepperDriver> stepper_d;

// Helper function to handle stepper motor control
void controlStepperMotor(int idx, float rpm) {
    // Validate motor index
    if (idx < 0 || idx >= NUM_MOTORS) {
        #if DEBUG_SERIAL
            Serial.print("Error: Invalid motor index: ");
            Serial.println(idx);
        #endif
        return;
    }

    #if DEBUG_SERIAL
        Serial.print("Control motor ");
        Serial.print((char)('A' + idx));
        Serial.print(": RPM=");
        Serial.print(rpm);
        Serial.print(", Direction=");
        Serial.println(motors[idx].direction ? "RIGHT" : "LEFT");
    #endif

    // Set direction pin
    digitalWrite(DIR_PINS[idx], motors[idx].direction);

    // Get the appropriate stepper instance
    auto& stepper = [&]() -> ContinuousStepper<StepperDriver>& {
        switch(idx) {
            case 0: return stepper_a;
            case 1: return stepper_b;
            case 2: return stepper_c;
            case 3: return stepper_d;
            default: return stepper_a; // Shouldn't happen due to bounds check
        }
    }();

    if (rpm > 0) {
        // Start the motor
        stepper.spin(rpm);
        #if DEBUG_SERIAL
            Serial.print("Started motor ");
            Serial.print((char)('A' + idx));
            Serial.print(" at ");
            Serial.print(rpm);
            Serial.println(" RPM");
        #endif
    } else {
        // Stop the motor
        stepper.stop();
        #if DEBUG_SERIAL
            Serial.print("Stopped motor ");
            Serial.println((char)('A' + idx));
        #endif
    }
}

String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        if (command.length() > 0) {
            processSerialCommand(command);
        }
    }
}

void processSerialCommand(const String& cmd) {
    String c = cmd;
    c.trim();
    c.toUpperCase();
    
    // Helper function to extract parameter value
    auto getParamValue = [](const String& str, const String& param) -> String {
        int index = str.indexOf(param);
        if (index < 0) return "";
        
        int start = index + param.length();
        int end = str.indexOf(' ', start);
        if (end < 0) end = str.length();
        
        String val = str.substring(start, end);
        val.trim();
        return val;
    };

    if (c.startsWith("SET ")) {
        String remainder = c.substring(4);
        remainder.trim();
        
        // Extract pump index (A, B, C, D)
        String pump = getParamValue(remainder, "PUMP=");
        if (pump.length() != 1) {
            Serial.println(RESPONSE_ERROR_CMD);
            return;
        }
        
        int idx = pump.charAt(0) - 'A';
        if (idx < 0 || idx >= NUM_MOTORS) {
            Serial.println(RESPONSE_ERROR_CMD_INVALID_PUMP);
            return;
        }
        
        bool success = false;
        bool should_recalculate_rpm = false;
        bool wasRunning = (motors[idx].state == RUN);
        String val;
        
        // FLOW
        val = getParamValue(remainder, "FLOW=");
        if (val.length() > 0) {
            motors[idx].flow = val.toInt();
            should_recalculate_rpm = true;
            success = true;
        }
        
        // DIAMETER
        val = getParamValue(remainder, "DIAMETER=");
        if (val.length() > 0) {
            motors[idx].diameter = val.toFloat();
            should_recalculate_rpm = true;
            success = true;
        }
        
        // UNIT
        val = getParamValue(remainder, "UNIT=");
        if (val.length() > 0) {
            if (val.startsWith("UL/MIN")) motors[idx].unit_type = UNIT_UL_MIN;
            else if (val.startsWith("UL/HR")) motors[idx].unit_type = UNIT_UL_HR;
            else if (val.startsWith("ML/MIN")) motors[idx].unit_type = UNIT_ML_MIN;
            else if (val.startsWith("ML/HR")) motors[idx].unit_type = UNIT_ML_HR;
            should_recalculate_rpm = true; 
            success = true;
        }

        // ROD (Thread Rod)
        val = getParamValue(remainder, "ROD=");
        if (val.length() > 0) {
            if (val.startsWith("1-START")) motors[idx].rod_type = ROD_1_STAR;
            else if (val.startsWith("4-START")) motors[idx].rod_type = ROD_4_STAR;
            should_recalculate_rpm = true; 
            success = true;
        }

        // GEARBOX
        val = getParamValue(remainder, "GEARBOX=");
        if (val.length() > 0) {
            if (val.startsWith("1:1")) motors[idx].gearbox_type = GEAR_BOX_1_1;
            else if (val.startsWith("25:1")) motors[idx].gearbox_type = GEAR_BOX_1_25;
            else if (val.startsWith("100:1")) motors[idx].gearbox_type = GEAR_BOX_1_100;
            should_recalculate_rpm = true; 
            success = true;
        }

        // MICROSTEP
        val = getParamValue(remainder, "MICROSTEP=");
        if (val.length() > 0) {
            if (val.startsWith("1/8")) motors[idx].microstep_type = MICRO_STEP_1_8;
            else if (val.startsWith("1/16")) motors[idx].microstep_type = MICRO_STEP_1_16;
            else if (val.startsWith("1/32")) motors[idx].microstep_type = MICRO_STEP_1_32;
            else if (val.startsWith("1/64")) motors[idx].microstep_type = MICRO_STEP_1_64;
            should_recalculate_rpm = true; 
            success = true;
        }

        // ENABLE
        val = getParamValue(remainder, "ENABLE=");
        if (val.length() > 0) {
            if (val.startsWith("ON")) {
                ioport.digitalWrite(EN_PINS[idx], LOW);  // Enable motor
                motors[idx].enabled = true;
                success = true;
            } else if (val.startsWith("OFF")) {
                ioport.digitalWrite(EN_PINS[idx], HIGH);  // Disable motor
                motors[idx].enabled = false;
                success = true;
            }
        }

        if (should_recalculate_rpm) {
            calculateNewStep(idx);
            saveMotorPreset(idx);
            // If motor is running, update its speed immediately
            if (motors[idx].state == RUN) {
                controlStepperMotor(idx, motors[idx].rpm);
            }
        }

        // DIRECTION
        val = getParamValue(remainder, "DIRECTION=");
        if (val.startsWith("INFUSE")) {
            motors[idx].direction = RIGHT_DIR;
            if (motors[idx].state == RUN) {
                controlStepperMotor(idx, motors[idx].rpm);
            } else {
                digitalWrite(DIR_PINS[idx], RIGHT_DIR);
            }
            success = true;
        } else if (val.startsWith("WITHDRAW")) {
            motors[idx].direction = LEFT_DIR;
            if (motors[idx].state == RUN) {
                controlStepperMotor(idx, motors[idx].rpm);
            } else {
                digitalWrite(DIR_PINS[idx], LEFT_DIR);
            }
            success = true;
        }
        
        // STATE
        val = getParamValue(remainder, "STATE=");
        if (val.startsWith("RUN")) {          
            motors[idx].state = RUN;
            ioport.digitalWrite(EN_PINS[idx], LOW);  // Enable motor
            controlStepperMotor(idx, motors[idx].rpm);
            success = true;
        } else if (val.startsWith("STOP")) {
            motors[idx].state = STOP;
            controlStepperMotor(idx, 0);  // Stop the motor
            ioport.digitalWrite(EN_PINS[idx], HIGH);  // Disable motor
            success = true;
        }
                        
        // Process results
        if (success) {
            if (idx == (int)actual_motor) {
                printScreen();
            }
            Serial.println(RESPONSE_OK);
        } else {
            Serial.println(RESPONSE_ERROR_CMD);
        }
    } 
    
    // -------------------- GET COMMAND --------------------
    else if (c.startsWith("GET ")) {
        String remainder = c.substring(4);
        remainder.trim();
        int pIndex = remainder.indexOf("PUMP=");
        if (pIndex < 0) {
            Serial.println(RESPONSE_ERROR_CMD_MISSING_PUMP);
            return;
        }
        String pump = remainder.substring(pIndex + 5, pIndex + 6);
        pump.trim();
        int idx = pump.charAt(0) - 'A';
        if (idx < 0 || idx >= NUM_MOTORS) {
            Serial.println(RESPONSE_ERROR_CMD_INVALID_PUMP);
            return;
        }
        if (remainder.indexOf("STATUS") >= 0) {
            char status[192];
            const char* unitStr = (motors[idx].unit_type == UNIT_UL_MIN) ? "UL/MIN" :
                                 (motors[idx].unit_type == UNIT_UL_HR)  ? "UL/HR"  :
                                 (motors[idx].unit_type == UNIT_ML_MIN) ? "ML/MIN" : "ML/HR";
            const char* gbStr   = (motors[idx].gearbox_type == GEAR_BOX_1_1)   ? "1:1"   :
                                 (motors[idx].gearbox_type == GEAR_BOX_1_25)  ? "25:1" : "100:1";
            const char* msStr   = (motors[idx].microstep_type == MICRO_STEP_1_8)  ? "1/8"  :
                                 (motors[idx].microstep_type == MICRO_STEP_1_16) ? "1/16" :
                                 (motors[idx].microstep_type == MICRO_STEP_1_32) ? "1/32" : "1/64";
            const char* rodStr  = (motors[idx].rod_type == ROD_1_STAR) ? "1-START" : "4-START";
            const char* stateStr = (motors[idx].state == RUN) ? "RUN" : "STOP";
            const char* dirStr  = (motors[idx].direction == RIGHT_DIR) ? "INFUSE" : "WITHDRAW";
            
            snprintf(status, sizeof(status),
                     "PUMP=%c FLOW=%d DIAMETER=%.2f DIRECTION=%s STATE=%s UNIT=%s GEARBOX=%s MICROSTEP=%s ROD=%s ENABLE=%s",
                     pump.charAt(0),
                     motors[idx].flow,
                     motors[idx].diameter,
                     dirStr,
                     stateStr, 
                     unitStr, 
                     gbStr, 
                     msStr, 
                     rodStr, 
                     (digitalRead(EN_PINS[idx]) == LOW) ? "ON" : "OFF");
            Serial.println(status);
        } else {
            Serial.println("ERROR_CMD_UNKNOWN_QUERY");
        }
    } else {
        // Unrecognized command
        Serial.println(RESPONSE_ERROR_CMD);
    }
}
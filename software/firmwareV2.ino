// Library inclusions
#include <LiquidCrystal_I2C.h>  // Library for controlling the LCD display
#include <TMCStepper.h>         // Library for controlling stepper motors
#include <SoftwareSerial.h>     // Library for serial communication

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Initialize LCD (address, columns, rows)

/**********************************************************************
 * PIN Configuration
 * Note: These pins correspond to the GP pins on the Raspberry Pi Pico
 **********************************************************************/

// LCD Pins
constexpr int LCD_SERIAL_DATA_LINE_PIN = 4;
constexpr int LCD_SERIAL_CLOCK_LINE_PIN = 5;
constexpr int LED_PIN_NUMBER = 25;

// Keypad Pins
const int pinRows[4] = {15, 14, 13, 12};
const int pinCols[4] = {11, 10, 9, 8};

// Pump Pins (A, B, C)
constexpr int PUMP_A_POWER = 16;
constexpr int PUMP_A_DIR_PIN = 17;
constexpr int PUMP_A_STEP_PIN = 18;
constexpr int PUMP_A_RX_PIN = 19;
constexpr int PUMP_A_TX_PIN = 20;
constexpr int PUMP_A_EN_PIN = 21;
SoftwareSerial serialPumpA(PUMP_A_TX_PIN, PUMP_A_RX_PIN);  // RX, TX for Pump A

// Similar definitions for PUMP_B and PUMP_C ...

constexpr int DRIVER_ADDRESS = 0b00;  // TMC2209 and TMC2226 Driver address (MS1 and MS2 configuration)
constexpr float R_SENSE = 0.1f;       // Current sensing resistance, match to your driver specs

// Motor driver setup for each pump - we use the driver module TMC2226 but the supplier software is still releaased for TMC2209.
TMC2209Stepper driverPumpA(&serialPumpA, R_SENSE, DRIVER_ADDRESS);
TMC2209Stepper driverPumpB(&serialPumpB, R_SENSE, DRIVER_ADDRESS);
TMC2209Stepper driverPumpC(&serialPumpC, R_SENSE, DRIVER_ADDRESS);
TMC2209Stepper* driver[3] = {&driverPumpA, &driverPumpB, &driverPumpC};

/**********************************************************************
 * VARIABLES
 **********************************************************************/

// Keypad configuration
constexpr char KEYPAD_KEYS[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// Rest of the code...
    float speed = ((((float)flow/(float)3600)/(float)a);  // mm/s Speed calcualtion: Input flowrates are in uL/h, they are converted into uL/s. 1uL (water) corresponds to 1mm^3. It is then divided by the area of the syringe (in mm^2) to obtain a distance in mm. Result in mm/s
    speed *= (float)stepsPerMm / (float)1000000); //conversion of units to mm/ms and incorporate stepp distance (to actually just get 1/ms)
    int t = (float)1/speed; // convert 1/ms to ms
    t = t/microSteps; // shorten step time depending on microstep usage
    return t;
}

// Function to control pump movement
void movePump() {
    // Logic to control the movement of the pump(s) based on current settings
    for (int i=0;i<3;i++){
        digitalWrite(pump[i]->pumpDirPin, pump[i]->direction);
        int stepTime = calculateStepTime(pump[i]->flowRate, pump[i]->diameter)/2;
        long time = micros();
        if ((time-pump[i]->timePump) >= stepTime && pump[i]->start){
            pump[i]->stepState = !pump[i]->stepState;
            digitalWrite(pump[i]->pumpStepPin, pump[i]->stepState);
            pump[i]->timePump = micros();
            Serial.println(String(stepTime));
        }
    }

}

// End of the code
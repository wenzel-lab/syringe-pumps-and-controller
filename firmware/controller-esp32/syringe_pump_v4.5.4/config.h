#ifndef CONFIG_H
#define CONFIG_H
#define FIRMWARE_VERSION "4.5.4"

#include <Arduino.h>
#include <Wire.h>
#include <PCF8574.h>
#include <LiquidCrystal_I2C.h>
#include <I2CKeyPad.h>
#include "clsPCA9555.h"
#include <TMCStepper.h>
#include <ContinuousStepper.h>
#include <Preferences.h>
#include <HardwareSerial.h>

// Forward declarations
class LiquidCrystal_I2C;
class I2CKeyPad;
class PCA9555;
class Preferences;


// Global instances
extern LiquidCrystal_I2C lcd;
extern I2CKeyPad keypad;
extern PCA9555 ioport;
extern Preferences preferences;

// Microstep configuration
struct MicrostepConfig {
    uint8_t ms1;
    uint8_t ms2;
};

enum MicrostepIndex {
    MS_EIGHTH_STEP = 0,
    MS_SIXTEENTH_STEP = 1,
    MS_THIRTYSECOND_STEP = 2,
    MS_SIXTYFOURTH_STEP = 3
};

// Debug Settings
#ifndef DEBUG_SERIAL
#define DEBUG_SERIAL 0  // Set to 1 to enable verbose debug messages
#endif

// I2C Addresses
#define GPIO_ADDR 0x20  // PCA9555 I/O Expander
#define LCD_ADDR 0x27   // LCD I2C Address
#define KEYPAD_ADDR 0x24 // Keypad I2C Address

#define DRIVER_ADDR_A 0b00 // TCM2209 Address
#define DRIVER_ADDR_B 0b01 // TCM2209 Address
#define DRIVER_ADDR_C 0b10 // TCM2209 Address
#define DRIVER_ADDR_D 0b11 // TCM2209 Address

//HArdware serial to PDN command controll on driver TCM2209
#define RXD2 17
#define TXD2 16
#define R_SENSE 0.11f

// Pin Configuration - defined in config.cpp
extern const int EN_PINS[4];     // Enable pins for motors A-D
extern const int STEP_PINS[4];   // Step pins for motors A-D
extern const int DIR_PINS[4];    // Direction pins for motors A-D
extern const int MS1_PINS[4];    // MS1 pins for motors A-D
extern const int MS2_PINS[4];    // MS2 pins for motors A-D
extern const MicrostepConfig MICROSTEP_CONFIGS[4];
extern const char MOTORS_NAMES[4]; // Motor identifiers

// Stepper motor steps per revolution (1.8° per step = 200 steps/rev)
#define STEP_PER_REV 200

// Keypad interrupt GPIO pin
#define KEYPAD_INT_PIN 18

// Microstepping Settings
enum {
    MICRO_STEP_1_8 = 8,
    MICRO_STEP_1_16 = 16,
    MICRO_STEP_1_32 = 32,
    MICRO_STEP_1_64 = 64
};

// Motor Control Constants
enum {
    RUN = 1,
    STOP = 0,
    RIGHT_DIR = 1,  // Infuse direction
    LEFT_DIR = 0    // Withdraw direction
};

// Motor Indices
enum {
    MOTOR_A = 0,
    MOTOR_B = 1,
    MOTOR_C = 2,
    MOTOR_D = 3,
    NUM_MOTORS = 4
};

// Gearbox Ratios
enum {
    GEAR_BOX_1_1 = 1,
    GEAR_BOX_1_25 = 25,
    GEAR_BOX_1_100 = 100
};

// Lead Screw Pitches (tenths of mm for better precision)
enum {
    ROD_1_STAR = 20,  // 2.0 mm pitch (stored as 20 = 2.0 * 10)
    ROD_4_STAR = 80   // 8.0 mm pitch (stored as 80 = 8.0 * 10)
};

// Syringe Volume Units
enum {
    UNIT_UL_MIN = 1000,       // 1 uL/min → multiply by 1000/1000 = 1.0
    UNIT_UL_HR = 17,          // 1 uL/hr → multiply by 17/1000 ≈ 0.0167
    UNIT_ML_MIN = 1000000,    // 1 mL/min → multiply by 1000000/1000 = 1000.0
    UNIT_ML_HR = 16667        // 1 mL/hr → multiply by 16667/1000 ≈ 16.667
};

// UI State Machine
enum {
    IDLE = 0,
    SETTINGS = 1,
    N_STATES = 34
};

// UI Element Types
enum listOfItemTypes {
    ACTIVITY,
    TEXT,
    LIST,
    DATA,
    LIST_MOTOR,
    LIST_FILL
};

// Motor structure
struct Motor {
    char name;
    int step_pin, dir_pin, en_pin, steps;
    int flow, gearbox_type, microstep_type, rod_type;
    float diameter, rpm;
    uint32_t unit_type;
    bool direction;  // true = RIGHT_DIR, false = LEFT_DIR
    bool state;      // true = RUN, false = STOP
    bool enabled;    // true = ENABLED, false = DISABLED
    bool initialized; // Tracks if motor is initialized
};

// Menu States structure
struct MenuStates {
    uint8_t button[5];
    uint8_t type;
    char *text;
};

// Global variables
extern volatile bool key_change;
extern const char keyMap[16];
extern String num_input;
extern uint8_t current_state;
extern uint8_t next_state;
extern char actual_motor;
extern char key;
extern const char* response;

extern void controlStepperMotor(int idx, float rpm);

// Function declarations
char getKey();
void calculateNewStep(int motor_idx = -1);
void processKey(char _key);
void printScreen();
void keyChanged();

void saveMotorPreset(int index);
void loadMotorPresets(int index);
void setMicrostepping(int motor_index, int microstep_value);

void initSerial();
void initLCD();
bool initKeypad();
void initMotorPresets();
void initMotorHardware();
void initStateMachine();
void initStepperDrivers();

#endif // CONFIG_H
/*
 Version of code: V4.5 - USB Serial Only

*/

// ──────────────────────────────────────────────────────────────
//  SECTION 0 ‒ LIBRARY INCLUDES & CORE DEFINES
// ──────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <Wire.h>
#include <PCF8574.h>
#include <LiquidCrystal_I2C.h>
#include <I2CKeyPad.h>
#include "clsPCA9555.h"
#include <ContinuousStepper.h>
#include <Preferences.h>

// ──────────────────────────────────────────────────────────────
//  SECTION 1 ‒ HARDWARE CONFIGURATION
// ──────────────────────────────────────────────────────────────

// Debug Settings
#ifndef DEBUG_SERIAL
#define DEBUG_SERIAL 0  // Set to 1 to enable verbose debug messages
#endif

// I2C Addresses
#define GPIO_ADDR 0x20  // PCA9555 I/O Expander
#define LCD_ADDR 0x27   // LCD I2C Address
#define KEYPAD_ADDR 0x24 // Keypad I2C Address (same as GPIO, different register)

// Keypad interrupt pin - must be a valid interrupt-capable pin on ESP32
// On most ESP32 boards, all GPIO pins can be used for interrupts
#define KEYPAD_INT_PIN 5  

// Motor Pin Configuration
const int EN_PINS[]   = {15, 11,  7,  3};  // Enable pins for motors A-D
const int STEP_PINS[] = { 4, 17, 26, 33};  // Step pins for motors A-D
const int DIR_PINS[]  = {16, 18, 25, 32};  // Direction pins for motors A-D
const char MOTORS_NAMES[] = {'A','B','C','D'};  // Motor identifiers

// Stepper motor steps per revolution (1.8° per step = 200 steps/rev)
#define STEP_PER_REV 200

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
    ROD_1_STAR = 15,  // 25 - 1.5mm pitch (stored as 15 = 1.5 * 10)
    ROD_4_STAR = 80   // 8.0mm pitch (stored as 80 = 8.0 * 10)
};

// Microstepping Settings
enum {
    MICRO_STEP_1_8 = 8,
    MICRO_STEP_1_16 = 16,
    MICRO_STEP_1_32 = 32,
    MICRO_STEP_1_64 = 64
};

// Syringe Volume Units (uL) - stored as integer * 1000 for precision
// This allows us to avoid floating point in enums while maintaining precision
enum {
    UNIT_UL_MIN = 1000,       // 1.0 uL/min (stored as 1000 = 1.0 * 1000)
    UNIT_UL_HR = 17,          // 0.016666... uL/min (stored as 17 = 1.0/60 * 1000, rounded)
    UNIT_ML_MIN = 1000000,    // 1000.0 uL/min (stored as 1000000 = 1000.0 * 1000)
    UNIT_ML_HR = 16667        // 16.666... uL/min (stored as 16667 = 1000.0/60 * 1000, rounded)
};

// UI State Machine
enum {
    IDLE = 0,
    SETTINGS = 1,
    N_STATES = 34
};

// Default Settings
#define FLOW_DEFAULT 		1000.0f        // Default flow rate in uL/h
#define DIAMETER_DEFAULT 	8.17f          // Default syringe diameter in mm
#define GEARBOX_DEFAULT 	GEAR_BOX_1_1
#define ROD_DEFAULT 		ROD_1_STAR
#define MICROSTEP_DEFAULT 	MICRO_STEP_1_16
#define UNIT_DEFAULT 		UNIT_UL_HR
#define DIR_DEFAULT		    RIGHT_DIR
#define STATE_DEFAULT		true

// Firmware and Device Information
#define VERSION 			"SP-V4.5-USB"
#define DEVICE_ID           "SP‑01"

// Serial Command Responses
#define RESPONSE_OK      	"OK"
#define RESPONSE_ERROR   	"ERROR"
#define RESPONSE_ERROR_JSON "ERROR_INPUT_JSON"
#define RESPONSE_ERROR_CMD  "ERROR_CMD"

// UI Element Types
enum listOfItemTypes {
    ACTIVITY,
    TEXT,
    LIST,
    DATA,
    LIST_MOTOR,
    LIST_FILL
};

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
I2CKeyPad         keypad(KEYPAD_ADDR);
PCA9555           ioport(GPIO_ADDR);

// Preferences (NVS)
Preferences preferences;

// Stepper objects (ContinuousStepper)
ContinuousStepper<StepperDriver> stepper_a;
ContinuousStepper<StepperDriver> stepper_b;
ContinuousStepper<StepperDriver> stepper_c;
ContinuousStepper<StepperDriver> stepper_d;

// ──────────────────────────────────────────────────────────────
//  SECTION 2 ‒ STRUCTS, GLOBAL VARIABLES & STATE ARRAYS
// ──────────────────────────────────────────────────────────────

struct MenuStates
{
	uint8_t button[5];
	uint8_t type;
	char *text;
};

struct Motor {
    char name;
    int step_pin;
    int dir_pin;
    int en_pin;
    int steps;
    float rpm;
    float flow;
    float diameter;
    int gearbox_type;
    float rod_type;
    int microstep_type;
    float unit_type;
    bool direction;  // true = RIGHT_DIR, false = LEFT_DIR
    bool state;      // true = RUN, false = STOP
    bool enabled;    // true = ENABLED, false = DISABLED
    bool initialized; // Add this line to track initialization
};

// Keypad and UI state
volatile bool key_change = false;
const char keyMap[16] = { '1','2','3','A',
                         '4','5','6','B',
                         '7','8','9','C',
                         '*','0','#','D' };

// State machine and motor control
MenuStates states[N_STATES];
Motor motors[NUM_MOTORS];

// UI navigation state
uint8_t next_state = 0;
uint8_t current_state = 0;
char    actual_motor = MOTOR_A;  // Currently selected motor (A-D)
char    key;                    // Last key pressed
String  num_input = "";         // Buffer for numeric input

// Serial communication
const char* response = RESPONSE_OK;

// ──────────────────────────────────────────────────────────────
//  SECTION 3 ‒ FORWARD DECLARATIONS
// ──────────────────────────────────────────────────────────────

char getKey();
void calculateNewStep();
void processKey(char _key);
void printScreen();
void keyChanged();

// Function declarations
void saveMotorPreset(int index);
void loadMotorPresets(int index);
void handleSerialCommands();
void processSerialCommand(const String& cmd);

// Initialization functions
void initSerial();
void initLCD();
bool initKeypad();
void initMotorPresets();
void initMotorHardware();
void initStateMachine();
void initStepperDrivers();

// ──────────────────────────────────────────────────────────────
//  SECTION 4 ‒ INITIALIZATION FUNCTIONS
// ──────────────────────────────────────────────────────────────

/**
 * Initialize serial communication and display welcome message
 */
void initSerial() {
    Serial.begin(115200);
 }

/**
 * Initialize the LCD display
 */
void initLCD() {
    Wire.begin();
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wenzel - Lab");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    delay(3000); // Brief pause for display
}

/**
 * Initialize the keypad with error checking
 * @return true if successful, false on error
 */
bool initKeypad() {
    // Initialize I2C for keypad
    Wire.begin();
    
    // Debug I2C scan
    Serial.println("Scanning I2C bus...");
    byte error, address;
    int nDevices = 0;
    for(address = 1; address < 127; address++ ) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            if (address<16) Serial.print("0");
            Serial.print(address,HEX);
            Serial.println("  ");
            nDevices++;
        }
    }
    if (nDevices == 0) {
        Serial.println("No I2C devices found!");
    }
    
    // Initialize keypad
    Serial.print("Initializing keypad at address 0x");
    Serial.println(KEYPAD_ADDR, HEX);
    
    // Initialize keypad
    if (!keypad.begin()) {
        Serial.println("\nERROR: Cannot communicate with keypad");
        lcd.clear();
        lcd.print("Keypad Error!");
        lcd.setCursor(0, 1);
        lcd.print("Check connection");
        return false;
    }
    
    // Set up keypad interrupt
    Serial.println("Keypad initialized successfully");
    Serial.print("Using interrupt pin: ");
    Serial.println(KEYPAD_INT_PIN);
    
    pinMode(KEYPAD_INT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(KEYPAD_INT_PIN), keyChanged, FALLING);
    key_change = false;
    
    return true;
}

/**
 * Main setup function
 */
void setup() {
    // Initialize hardware and peripherals
    initSerial();
    initLCD();
    
    // Initialize I/O expander
    if (!ioport.begin()) {
        Serial.println("\nERROR: Cannot communicate with I/O expander");
        lcd.clear();
        lcd.print("I/O Expander");
        lcd.setCursor(0, 1);
        lcd.print("Error!");
        while(1); // Halt on error
    }
    
    // Initialize keypad
    if (!initKeypad()) {
        // Keypad initialization failed, but continue with other functions
        Serial.println("WARNING: Keypad initialization failed, continuing without keypad");
        lcd.clear();
        lcd.print("Keypad Error!");
        lcd.setCursor(0, 1);
        lcd.print("Using Serial Only");
        delay(2000);
    }
    
    // Initialize motor presets and hardware
    initMotorPresets();
    initMotorHardware();
    
    // Initialize stepper drivers (all start stopped)
    initStepperDrivers();
    
    // Initialize state machine
    initStateMachine();
    
    // Clear display and show ready message
    lcd.clear();
    lcd.print("Ready");
    delay(1000);
    printScreen();
}

/**
 * Initialize motor presets from NVS or set defaults
 */
void initMotorPresets() {
    for (int i = 0; i < NUM_MOTORS; i++) {
        // Load saved presets
        loadMotorPresets(i);
        
        // Force all motors to be stopped, enabled, and set to INFUSE direction on reboot
        motors[i].state = STOP;           // Force STOP state
        motors[i].enabled = true;         // Enable the motor
        motors[i].direction = RIGHT_DIR;  // Default to INFUSE direction
        motors[i].initialized = false;
        
        // Save the updated state
        saveMotorPreset(i);
    }
}

/**
 * Initialize motor hardware pins and settings
 */
void initMotorHardware() {
    for (int i = 0; i < NUM_MOTORS; i++) {
        // Initialize motor name
        motors[i].name = MOTORS_NAMES[i];
        
        // Configure enable pins - set LOW to enable the driver (active low)
        ioport.pinMode(EN_PINS[i], OUTPUT);
        ioport.digitalWrite(EN_PINS[i], LOW);  // LOW = ENABLED
        
        // Configure direction and step pins
        pinMode(DIR_PINS[i], OUTPUT);
        pinMode(STEP_PINS[i], OUTPUT);
        
        // Set initial direction (INFUSE by default)
        digitalWrite(DIR_PINS[i], RIGHT_DIR);
        
        // Ensure motor is stopped
        digitalWrite(STEP_PINS[i], LOW);
    }
}

/**
 * Initialize the state machine
 */
void initStateMachine() {
    // Main menu and basic controls
    states[0] = {0, 0, 5, 1, 2, ACTIVITY, "MOTOR"};  // Main menu
    
    // Flow input
    states[1] = {1, 1, 0, 1, 1, DATA, "Flow"};
    
    // Direction selection
    states[2] = {2, 2, 0, 1, 3, LIST_FILL, "Infuse"};
    states[3] = {3, 3, 0, 1, 2, LIST_FILL, "Withdraw"};
    
    // Motor selection
    states[4] = {4, 4, 0, 1, 4, LIST_MOTOR, "A"};
    states[26] = {26, 26, 0, 1, 26, LIST_MOTOR, "B"};
    states[27] = {27, 27, 0, 1, 27, LIST_MOTOR, "C"};
    states[28] = {28, 28, 0, 1, 28, LIST_MOTOR, "D"};
    
    // Settings menu
    states[5] = {10, 6, 11, 1, 2, LIST, "Diameter"};
    states[6] = {5, 7, 13, 1, 2, LIST, "Unit"};
    states[7] = {6, 8, 17, 1, 2, LIST, "Gearbox"};
    states[8] = {7, 9, 20, 1, 2, LIST, "Microstep"};
    states[9] = {8, 10, 24, 1, 2, LIST, "Thread Rod"};
    states[10] = {9, 5, 29, 1, 2, LIST, "Enable"};
    
    // Diameter input
    states[11] = {11, 11, 0, 1, 2, DATA, "Diameter"};
    states[12] = {12, 12, 0, 1, 2, DATA, "Diameter OK"};
    
    // Unit selection
    states[13] = {16, 14, 0, 1, 2, LIST, "ul/min"};
    states[14] = {13, 15, 0, 1, 2, LIST, "ul/h"};
    states[15] = {14, 16, 0, 1, 2, LIST, "ml/min"};
    states[16] = {15, 13, 0, 1, 2, LIST, "ml/h"};
    
    // Gearbox selection
    states[17] = {19, 18, 0, 1, 2, LIST, "1:1"};
    states[18] = {17, 19, 0, 1, 2, LIST, "25:1"};
    states[19] = {18, 17, 0, 1, 2, LIST, "100:1"};
    
    // Microstep selection
    states[20] = {23, 21, 0, 1, 2, LIST, "1/8"};
    states[21] = {20, 22, 0, 1, 2, LIST, "1/16"};
    states[22] = {21, 23, 0, 1, 2, LIST, "1/32"};
    states[23] = {22, 20, 0, 1, 2, LIST, "1/64"};
    
    // Leadscrew selection
    states[24] = {25, 25, 0, 1, 2, LIST, "1-start"};
    states[25] = {24, 24, 0, 1, 2, LIST, "4-start"};
    
    // Motor enable/disable
    states[29] = {32, 30, 0, 1, 2, LIST, "A"};
    states[30] = {29, 31, 0, 1, 2, LIST, "B"};
    states[31] = {30, 32, 0, 1, 2, LIST, "C"};
    states[32] = {31, 29, 0, 1, 2, LIST, "D"};
}

/**
 * Initialize stepper motor drivers
 */
void initStepperDrivers() {
    stepper_a.begin(motors[0].step_pin, motors[0].dir_pin);
    stepper_b.begin(motors[1].step_pin, motors[1].dir_pin);
    stepper_c.begin(motors[2].step_pin, motors[2].dir_pin);
    stepper_d.begin(motors[3].step_pin, motors[3].dir_pin);
    
    // Start all steppers in stopped state
    stepper_a.spin(0);
    stepper_b.spin(0);
    stepper_c.spin(0);
    stepper_d.spin(0);
}

// State machine initialization is now handled in the initStateMachine() function above

// ──────────────────────────────────────────────────────────────
//  SECTION 5 – MAIN LOOP
// ──────────────────────────────────────────────────────────────

/**
 * Main program loop
 * Handles:
 * - Serial command processing
 * - Keypad input
 * - Motor control
 */
void loop() {
    
    // USB serial commands
    handleSerialCommands();
    
    // Process keypad input if available
    if (key_change) {
        key_change = false;
        char key = getKey();
        if (key != 'N' && key != 'F') {
            processKey(key);
        }
    }
    
    // Update stepper motor states
    stepper_a.loop();
    stepper_b.loop();
    stepper_c.loop();
    stepper_d.loop();

    // Small delay to prevent busy-waiting
    delay(1);
}

// ──────────────────────────────────────────────────────────────
//  SECTION 6 – UI HANDLERS (processKey, printScreen)
// ──────────────────────────────────────────────────────────────

// ---------------- getKey() ----------------
// Non-blocking keypad reader (legacy behavior)
char getKey()
{
    static const char keys[] = "123A456B789C*0#DNF";  // N = NoKey, F = Fail
    uint8_t index = keypad.getKey();
    // Library often returns 0x1F for "no key" on PCF8574
    if (index == 0x1F || index >= sizeof(keys) - 1) {
        return 'N';
    }
    return keys[index];
}

// ---------------- processKey() ----------------
void processKey(char _key)
{
  if(_key != 'N')
  {
		// * (STAR) toggles RUN / STOP
		if(_key == '*' and (current_state!= 1 and current_state != 11))
		{
			motors[actual_motor].state = !motors[actual_motor].state;
			
            if(motors[actual_motor].state) //RUN
			{
				ioport.digitalWrite(motors[actual_motor].en_pin, LOW);
				digitalWrite(motors[actual_motor].dir_pin, motors[actual_motor].direction);

				if( actual_motor == MOTOR_A){stepper_a.spin(motors[actual_motor].rpm);}
				if( actual_motor == MOTOR_B){stepper_b.spin(motors[actual_motor].rpm);}
				if( actual_motor == MOTOR_C){stepper_c.spin(motors[actual_motor].rpm);}
				if( actual_motor == MOTOR_D){stepper_d.spin(motors[actual_motor].rpm);}
			}
			else //STOP
			{
				ioport.digitalWrite(motors[actual_motor].en_pin, HIGH);
				if( actual_motor == MOTOR_A){stepper_a.spin(0); stepper_a.stop();}
				if( actual_motor == MOTOR_B){stepper_b.spin(0); stepper_b.stop();}
				if( actual_motor == MOTOR_C){stepper_c.spin(0); stepper_c.stop();}
				if( actual_motor == MOTOR_D){stepper_d.spin(0); stepper_d.stop();}
			}
		}
        if(current_state != 1 and current_state != 11)
        {
            if(_key == 'A')      { next_state = states[current_state].button[3]; }
            else if(_key == 'B') { next_state = states[current_state].button[4]; }
            else if(_key == 'C') { actual_motor = (actual_motor + 1) % 4; }
            else if(_key == '2') { next_state = states[current_state].button[0]; }
            else if(_key == '8') { next_state = states[current_state].button[1]; }
            else if(_key == '#') { next_state = states[current_state].button[2]; }
        }
		if(current_state == 1 or current_state == 11)
        {
            if(_key == '*')  // Clear input when <*> is pressed
            {
                num_input = "";
                if (current_state == 11)  // Diameter input
                {
                    motors[actual_motor].diameter = 0.0f;  // Set to 0 for diameter
                }
                else  // Flow input
                {
                    motors[actual_motor].flow = 0.0f;  // Set to 0 for flow
                }
                
                lcd.setCursor(0,1);
                lcd.print("                ");  // Clear the line
                lcd.setCursor(0,1);
            }
            else if(_key >= '0' && _key <= '9')      // digits always allowed
            {
                num_input += _key;
            }
            else if(_key == 'D'                // decimal point rules
                    && current_state == 11      // • only on DIAMETER screen
                    && num_input.indexOf('.') == -1)
            {
                if(num_input.length() == 0) num_input = "0.";
                else                         num_input += '.';
            }
            else if(_key == '#') { next_state = states[current_state].button[2];}

            // live echo of entry
            lcd.setCursor(0,1);
            lcd.print("                ");
            lcd.setCursor(0,1);
            lcd.print(num_input);
        }

		// ---------- MENU-ACTION on ENTER (next_state==0) ----------
		if(next_state == 0)
		{
			// unit/gearbox/microstep/rod selection
			if( (current_state > 12 and current_state < 26)  )
			{
				if (current_state == 13 ) motors[actual_motor].unit_type = UNIT_UL_MIN;
				if (current_state == 14 ) motors[actual_motor].unit_type = UNIT_UL_HR;
				if (current_state == 15 ) motors[actual_motor].unit_type = UNIT_ML_MIN;
				if (current_state == 16 ) motors[actual_motor].unit_type = UNIT_ML_HR;

				if (current_state == 17 ) motors[actual_motor].gearbox_type = GEAR_BOX_1_1;
				if (current_state == 18 ) motors[actual_motor].gearbox_type = GEAR_BOX_1_25;
				if (current_state == 19 ) motors[actual_motor].gearbox_type = GEAR_BOX_1_100;

				if (current_state == 20 ) motors[actual_motor].microstep_type = MICRO_STEP_1_8;
				if (current_state == 21 ) motors[actual_motor].microstep_type = MICRO_STEP_1_16;
				if (current_state == 22 ) motors[actual_motor].microstep_type = MICRO_STEP_1_32;
				if (current_state == 23 ) motors[actual_motor].microstep_type = MICRO_STEP_1_64;

				if (current_state == 24 ) motors[actual_motor].rod_type = ROD_1_STAR;
				if (current_state == 25 ) motors[actual_motor].rod_type = ROD_4_STAR;

				calculateNewStep();
				saveMotorPreset(actual_motor);
				lcd.setCursor(12,0);
				lcd.print(" Ok");
				delay(1000);
			}
			else if(current_state == 4 or current_state == 26 or current_state == 27 or current_state == 28)
			{
				if(current_state == 4)  actual_motor = MOTOR_A;
				if(current_state == 26) actual_motor = MOTOR_B;
				if(current_state == 27) actual_motor = MOTOR_C;
				if(current_state == 28) actual_motor = MOTOR_D;
				lcd.setCursor(2,1);
				lcd.print(" Ok");
				delay(1000);
			}
			else if(current_state == 2 or current_state == 3)
			{
				if( current_state == 2 ) motors[actual_motor].direction = true; // Infuse
				if( current_state == 3 ) motors[actual_motor].direction = false; // Withdraw
				
				// Set DIR pin based on direction ( Infuse vs Withdraw )
				digitalWrite(motors[actual_motor].dir_pin, motors[actual_motor].direction);
			}
			else if(current_state == 1) // target flow input
			{
                bool wasRunning = motors[actual_motor].state;
                
                if (num_input.length() > 0)
                {
                    motors[actual_motor].flow = num_input.toFloat();
		        }
				else
                {
                    motors[actual_motor].flow = motors[actual_motor].flow;
                }
                
                calculateNewStep();
                saveMotorPreset(actual_motor);
                
                // If motor was running, update its speed immediately
                if (wasRunning) {
                    switch(actual_motor) {
                        case MOTOR_A: stepper_a.spin(motors[actual_motor].rpm); break;
                        case MOTOR_B: stepper_b.spin(motors[actual_motor].rpm); break;
                        case MOTOR_C: stepper_c.spin(motors[actual_motor].rpm); break;
                        case MOTOR_D: stepper_d.spin(motors[actual_motor].rpm); break;
                    }
                }
                
                lcd.setCursor(0,1);
                lcd.print("                ");
                lcd.setCursor(0,1);
                lcd.print(int(motors[actual_motor].flow));
                lcd.setCursor(14,1);
                lcd.print("Ok");
                delay(1000);

                // Reset input buffer and return to main screen
                num_input = "";
                current_state = 0;
			}
			else if(current_state == 11) // diameter input
			{
                bool wasRunning = motors[actual_motor].state;
                
                if (num_input.length() > 0) {
                    motors[actual_motor].diameter = num_input.toFloat();
                }
				else {
                    // No input entered, keep current value
                    motors[actual_motor].diameter = motors[actual_motor].diameter;
                }
                
                calculateNewStep();
                saveMotorPreset(actual_motor);
                
                // If motor was running, update its speed immediately
                if (wasRunning) {
                    switch(actual_motor) {
                        case MOTOR_A: stepper_a.spin(motors[actual_motor].rpm); break;
                        case MOTOR_B: stepper_b.spin(motors[actual_motor].rpm); break;
                        case MOTOR_C: stepper_c.spin(motors[actual_motor].rpm); break;
                        case MOTOR_D: stepper_d.spin(motors[actual_motor].rpm); break;
                    }
                }
                
                lcd.setCursor(0,1);
                lcd.print("                ");
                lcd.setCursor(0,1);
                lcd.print(motors[actual_motor].diameter, 2);
                lcd.setCursor(14,1);
                lcd.print("Ok");
                delay(1000);

                // Reset input buffer and return to main screen
                num_input = "";
                current_state = 0;
			}
		}
		if(next_state == 1)
		{
			if (current_state != 1)
			{
				lcd.setCursor(1,1);
				lcd.print("              ");
			}
		}
		if(next_state == 11)
		{
			if (current_state != 11)
			{
				lcd.setCursor(1,1);
				lcd.print("              ");
			}
		}
		current_state = next_state;
		printScreen();
    }
}
// -------------------- printScreen() ----------------------
void printScreen()
{
	if(current_state == 0)
	{
		lcd.setCursor(0,0);  lcd.print("                ");
		lcd.setCursor(0,0);	 lcd.print(" PUMP");
		lcd.setCursor(8,0);  lcd.print("A");
		lcd.setCursor(10,0); lcd.print("B");
		lcd.setCursor(12,0); lcd.print("C");
		lcd.setCursor(14,0); lcd.print("D");
		switch (actual_motor)
		{
			case MOTOR_A:{lcd.setCursor(7,0);lcd.print(">");break;}
			case MOTOR_B:{lcd.setCursor(9,0);lcd.print(">");break;}
			case MOTOR_C:{lcd.setCursor(11,0);lcd.print(">");break;}
			case MOTOR_D:{lcd.setCursor(13,0);lcd.print(">");break;}
		}
		lcd.setCursor(0,1);
        lcd.print("                ");
		lcd.setCursor(1,1);
		if(motors[actual_motor].state){lcd.print("RUN"); }
		else{lcd.print("STOP");}
	}

	// FLOW-RATE entry screen
	if(current_state == 1)
	{
		lcd.setCursor(0,0);	 lcd.print("                ");
		lcd.setCursor(0,0);	 lcd.print("FLOW");
		lcd.setCursor(4,0);

        if(motors[actual_motor].unit_type == UNIT_UL_MIN) lcd.print("(uL/min):");
		else if(motors[actual_motor].unit_type == UNIT_UL_HR) lcd.print("(uL/hr):");
		else if(motors[actual_motor].unit_type == UNIT_ML_MIN) lcd.print("(mL/min):");
		else if(motors[actual_motor].unit_type == UNIT_ML_HR) lcd.print("(mL/hr):");

		lcd.setCursor(15,0); lcd.print(motors[actual_motor].name);
		lcd.setCursor(0,1);

		// Display current flow value if no input is being entered
		if (num_input.length() == 0) {
			lcd.print(int(motors[actual_motor].flow));
		}
	}

	// DIAMETER entry
	if(current_state == 11)
	{
		lcd.setCursor(0,0);	 lcd.print("                ");
		lcd.setCursor(0,0);	 lcd.print("DIAMETER(mm):");
		lcd.setCursor(15,0); lcd.print(motors[actual_motor].name);
		lcd.setCursor(0,1);

		// Display current diameter value if no input is being entered
		if (num_input.length() == 0) {
			lcd.print(motors[actual_motor].diameter, 2);
		}
	}

	// LIST & LIST_FILL redraws
	if (states[current_state].type == LIST)
	{
		lcd.clear();
		lcd.setCursor(15,0); lcd.print(motors[actual_motor].name);
		lcd.setCursor(0,0);  lcd.print("> ");
		lcd.print(states[current_state].text);

		if(current_state > 11 and current_state < 25)
		{
			if( current_state == 13 and motors[actual_motor].unit_type == UNIT_UL_MIN) lcd.print(" *");
			if( current_state == 14 and motors[actual_motor].unit_type == UNIT_UL_HR) lcd.print(" *");
			if( current_state == 15 and motors[actual_motor].unit_type == UNIT_ML_MIN) lcd.print(" *");
			if( current_state == 16 and motors[actual_motor].unit_type == UNIT_ML_HR) lcd.print(" *");

			if( current_state == 17 and motors[actual_motor].gearbox_type == GEAR_BOX_1_1) lcd.print(" *");
			if( current_state == 18 and motors[actual_motor].gearbox_type == GEAR_BOX_1_25) lcd.print(" *");
			if( current_state == 19 and motors[actual_motor].gearbox_type == GEAR_BOX_1_100) lcd.print(" *");

			if( current_state == 20 and motors[actual_motor].microstep_type == MICRO_STEP_1_8) lcd.print(" *");
			if( current_state == 21 and motors[actual_motor].microstep_type == MICRO_STEP_1_16) lcd.print(" *");
			if( current_state == 22 and motors[actual_motor].microstep_type == MICRO_STEP_1_32) lcd.print(" *");
			if( current_state == 23 and motors[actual_motor].microstep_type == MICRO_STEP_1_64) lcd.print(" *");

			if( current_state == 24 and motors[actual_motor].rod_type == ROD_1_STAR) lcd.print(" *");
			if( current_state == 25 and motors[actual_motor].rod_type == ROD_4_STAR) lcd.print(" *");
		}

		// show next-item on second row
    	if (states[current_state].button[1] < 200)
    	{
			lcd.setCursor(0,1);
			lcd.print("  ");
			lcd.print(states[states[current_state].button[1]].text);
			
			if(current_state > 11 and current_state < 25)
			{
				if( current_state == 16 and motors[actual_motor].unit_type == UNIT_UL_HR) lcd.print(" *");
				if( current_state == 13 and motors[actual_motor].unit_type == UNIT_ML_HR) lcd.print(" *");
				if( current_state == 14 and motors[actual_motor].unit_type == UNIT_UL_MIN) lcd.print(" *");
				if( current_state == 15 and motors[actual_motor].unit_type == UNIT_ML_MIN) lcd.print(" *");

				if( current_state == 19 and motors[actual_motor].gearbox_type == GEAR_BOX_1_1) lcd.print(" *");
				if( current_state == 17 and motors[actual_motor].gearbox_type == GEAR_BOX_1_25) lcd.print(" *");
				if( current_state == 18 and motors[actual_motor].gearbox_type == GEAR_BOX_1_100) lcd.print(" *");

				if( current_state == 23 and motors[actual_motor].microstep_type == MICRO_STEP_1_8) lcd.print(" *");
				if( current_state == 20 and motors[actual_motor].microstep_type == MICRO_STEP_1_16) lcd.print(" *");
				if( current_state == 21 and motors[actual_motor].microstep_type == MICRO_STEP_1_32) lcd.print(" *");
				if( current_state == 22 and motors[actual_motor].microstep_type == MICRO_STEP_1_64) lcd.print(" *");

				if( current_state == 25 and motors[actual_motor].rod_type == ROD_1_STAR) lcd.print(" *");
				if( current_state == 24 and motors[actual_motor].rod_type == ROD_4_STAR) lcd.print(" *");
			}
		}
	}
	
	if (states[current_state].type == LIST_FILL)
	{
		lcd.clear();
		lcd.setCursor(15,0); lcd.print(motors[actual_motor].name);
		lcd.setCursor(0,0);  lcd.print("> ");
		lcd.print(states[current_state].text);

		if(current_state == 2 and motors[actual_motor].direction == RIGHT_DIR ) lcd.print(" *");
		if(current_state == 3 and motors[actual_motor].direction == LEFT_DIR ) lcd.print(" *");

		lcd.setCursor(0,1);
		lcd.print("  ");
		lcd.print(states[states[current_state].button[4]].text);

		if(current_state == 3 and motors[actual_motor].direction == RIGHT_DIR ) lcd.print(" *");
		if(current_state == 2 and motors[actual_motor].direction == LEFT_DIR ) lcd.print(" *");
	}
}

// ──────────────────────────────────────────────────────────────
//  SECTION 8 – MOTION CALCS
// ──────────────────────────────────────────────────────────────

void calculateNewStep()
{
    // 1) Grab the raw parameters for the currently selected motor
    float _flow            = motors[actual_motor].flow;           // e.g., "3000" if typed as 3000 μL/h
    float _diameter        = motors[actual_motor].diameter;       // e.g., 8.17 mm
    int   _gearbox         = motors[actual_motor].gearbox_type;   // e.g., 25 for a 1:25 gearbox
    int   _rod             = motors[actual_motor].rod_type;       // Stored as tenths of mm (e.g., 15 for 1.5mm)
    int   _microstep       = motors[actual_motor].microstep_type;  // e.g., 16 for 1/16 microstep
    int   _unit            = motors[actual_motor].unit_type;       // Stored as integer * 1000 (e.g., 17 for 0.017)in μL/h

    // 2) Convert the desired flow rate to μL/min (if it's not already):
    //    desired_flow_ul_per_min = _flow (in user's unit) × (_unit / 1000.0) (conversion to μL/min)
    // Note: _unit is stored as integer * 1000, so we divide by 1000.0 to get the actual unit value
    float unit_scale = _unit / 1000.0f;
    float desired_flow_ul_per_min = _flow * unit_scale;

    // 3) Compute syringe cross‐sectional area (in mm²):
    //    A = π · (D/2)²
    float radius_mm = _diameter / 2.0f;
    float cross_section_mm2 = PI * radius_mm * radius_mm;   // mm²

    // 4) Compute how many μL come out per full revolution of the leadscrew:
    //    vol_per_rev_ul = cross_section_mm2 (mm²) × rod_lead_mm (mm/rev)
    //                   = mm³/rev = μL/rev
    // Convert _rod from tenths of mm to mm by dividing by 10.0
    float rod_pitch_mm = _rod / 10.0f;
    float vol_per_rev_ul = cross_section_mm2 * rod_pitch_mm;

    // 5) Now compute the needed RPM (revolutions per minute):
    //    RPM = (desired_flow_ul_per_min) / (vol_per_rev_ul)
    float motor_rpm = desired_flow_ul_per_min / vol_per_rev_ul;
    motor_rpm *= _gearbox;
    motor_rpm *= 16.0f; // replace it by _microstep when it's available

    // 6) Compute how many steps correspond to one full revolution of the motor:
    //    e.g. 200 × 16 × 1  = 3200 steps/rev if gearbox = 1:1 & 1/16 microstep
    // float motor_steps_per_rev = STEP_PER_REV * _microstep * _gearbox;
    // float motor_steps_per_rev = STEP_PER_REV * 16.0f * _gearbox;

    // 7) Convert RPM → steps per minute:
    //    steps_per_min = motor_rpm × steps_per_rev
    // float steps_per_min = motor_rpm * motor_steps_per_rev;

    // 8) If you need “steps per second” (for many stepper libraries), divide by 60:
    // float steps_per_sec = steps_per_min / 60.0f;

    // 9) Finally, store these back into your motor struct:
    //    If your code expects “motors[].rpm” to hold the RPM value:
    motors[actual_motor].rpm = motor_rpm;

    // If your code expects “motors[].steps” to hold “steps per second,” then:
    // motors[actual_motor].steps = int(round(steps_per_sec));

    // 10) Persist the settings to EEPROM (if desired) and print debug info:
    saveMotorPreset(actual_motor);
}

// ---------------- ISR – keypad interrupt ---------------------
void IRAM_ATTR keyChanged(){
	key_change = true;
}


// ──────────────────────────────────────────────────────────────
//  SECTION 9 – PRESET STORAGE & NETWORKING UTILITIES
// ──────────────────────────────────────────────────────────────

// ---------------- saveMotorPreset -----------------
void saveMotorPreset(int index)
{
    String ns = "motor_" + String(MOTORS_NAMES[index]);
    preferences.begin(ns.c_str(), false);  // write mode
    
    preferences.putChar  ("name",   motors[index].name);
    preferences.putFloat ("flow",   motors[index].flow);
    preferences.putFloat ("diam",   motors[index].diameter);
    preferences.putChar  ("gear",   motors[index].gearbox_type);
    preferences.putChar  ("rod",    motors[index].rod_type);
    preferences.putChar  ("micro",  motors[index].microstep_type);
    preferences.putChar  ("unit", motors[index].unit_type);
    preferences.putBool  ("dir",    motors[index].direction);
    preferences.putBool  ("state",  motors[index].state);

    preferences.end();
}

// ---------------- loadMotorPresets ----------------
void loadMotorPresets(int index)
{
  String ns = "motor_" + String(MOTORS_NAMES[index]);
  preferences.begin(ns.c_str(), true);

  motors[index].name            = preferences.getChar  ("name",   MOTORS_NAMES[index]);
  motors[index].flow            = preferences.getFloat ("flow", FLOW_DEFAULT);
  motors[index].diameter        = preferences.getFloat ("diam", DIAMETER_DEFAULT);
  motors[index].gearbox_type    = preferences.getChar  ("gear", GEARBOX_DEFAULT);
  motors[index].rod_type        = preferences.getChar  ("rod", ROD_DEFAULT);
  motors[index].microstep_type  = preferences.getChar  ("micro", MICROSTEP_DEFAULT);
  motors[index].unit_type       = preferences.getChar  ("unit", UNIT_DEFAULT);
  motors[index].direction       = preferences.getBool  ("dir", DIR_DEFAULT);
  motors[index].state           = preferences.getBool  ("state", STATE_DEFAULT);
  
  preferences.end();
}

// ──────────────────────────────────────────────────────────────
//  SECTION 10 - Serial Command Handlers
// ──────────────────────────────────────────────────────────────

/**
 * Print status of one or all motors
 * @param motorStr Motor ID (A-D) or empty for all motors
 */

// Helper to trim and split strings
String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for(int i=0; i<=maxIndex && found<=index; i++){
        if(data.charAt(i)==separator || i==maxIndex){
            found++;
            strIndex[0] = strIndex[1]+1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void handleSerialCommands() {
    static String inputString = "";
    while (Serial.available()) {
        char inChar = (char)Serial.read();
        if (inChar == '\n' || inChar == '\r') {
            if (inputString.length() > 0) {
                processSerialCommand(inputString);
                inputString = "";
            }
        } else {
            inputString += inChar;
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
        if (idx < 0 || idx > 3) {
            Serial.println(RESPONSE_ERROR_CMD);
            return;
        }
        
        bool success = false;
        bool should_recalculate_rpm = false;
        bool wasRunning = (motors[idx].state == RUN);
        String val;
        
        // FLOW
        val = getParamValue(remainder, "FLOW=");
        if (val.length() > 0) {
            motors[idx].flow = val.toFloat();
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
            should_recalculate_rpm = true; success = true;
        }

        // ROD (Thread Rod)
        val = getParamValue(remainder, "ROD=");
        if (val.length() > 0) {
            if (val.startsWith("1-START")) motors[idx].rod_type = ROD_1_STAR;
            else if (val.startsWith("4-START")) motors[idx].rod_type = ROD_4_STAR;
            should_recalculate_rpm = true; success = true;
        }

        // GEARBOX
        val = getParamValue(remainder, "GEARBOX=");
        if (val.length() > 0) {
            if (val.startsWith("1:1")) motors[idx].gearbox_type = GEAR_BOX_1_1;
            else if (val.startsWith("25:1")) motors[idx].gearbox_type = GEAR_BOX_1_25;
            else if (val.startsWith("100:1")) motors[idx].gearbox_type = GEAR_BOX_1_100;
            should_recalculate_rpm = true; success = true;
        }

        // MICROSTEP
        val = getParamValue(remainder, "MICROSTEP=");
        if (val.length() > 0) {
            if (val.startsWith("1/8")) motors[idx].microstep_type = 8;
            else if (val.startsWith("1/16")) motors[idx].microstep_type = 16;
            else if (val.startsWith("1/32")) motors[idx].microstep_type = 32;
            else if (val.startsWith("1/64")) motors[idx].microstep_type = 64;
            should_recalculate_rpm = true; success = true;
        }

        if (should_recalculate_rpm) {
            calculateNewStep();
            saveMotorPreset(idx);
        }

        // DIRECTION
        val = getParamValue(remainder, "DIRECTION=");
        if (val.startsWith("INFUSE")) {
            motors[idx].direction = RIGHT_DIR;
            success = true;
        } else if (val.startsWith("WITHDRAW")) {
            motors[idx].direction = LEFT_DIR;
            success = true;
        }
        
        // STATE
        val = getParamValue(remainder, "STATE=");
        if (val.startsWith("RUN")) {
            // Add initialization check
            if (!motors[idx].initialized) {
                    // Initialize motor control pins
                    ioport.digitalWrite(EN_PINS[idx], HIGH);  // Start with motor disabled
                    digitalWrite(DIR_PINS[idx], motors[idx].direction);
                    pinMode(STEP_PINS[idx], OUTPUT);
                    motors[idx].initialized = true;
                }   

            motors[idx].state = RUN;
            ioport.digitalWrite(EN_PINS[idx], LOW);  // Enable motor driver
            digitalWrite(DIR_PINS[idx], motors[idx].direction);

            // Start the appropriate stepper
            switch(idx) {
                case 0: stepper_a.spin(motors[idx].rpm); break;
                case 1: stepper_b.spin(motors[idx].rpm); break;
                case 2: stepper_c.spin(motors[idx].rpm); break;
                case 3: stepper_d.spin(motors[idx].rpm); break;
            }
            success = true;
        } else if (val.startsWith("STOP")) {
            
            motors[idx].state = STOP;
            ioport.digitalWrite(EN_PINS[idx], HIGH);  // Disable motor driver
            digitalWrite(DIR_PINS[idx], motors[idx].direction);
            
            
            // Stop the appropriate stepper
            switch(idx) {
                case 0: stepper_a.spin(0); stepper_a.stop(); break;
                case 1: stepper_b.spin(0); stepper_b.stop(); break;
                case 2: stepper_c.spin(0); stepper_c.stop(); break;
                case 3: stepper_d.spin(0); stepper_d.stop(); break;
            }
             success = true;
        }
                        
        // Process results
        if (success) {
            if (idx == actual_motor) printScreen(); // Update display if needed
            Serial.println(RESPONSE_OK);
        }
        else { Serial.println(RESPONSE_ERROR_CMD);  }
    } 
    
    // -------------------- GET COMMAND --------------------
    else if (c.startsWith("GET ")) {
        String remainder = c.substring(4);
        remainder.trim();
        int pIndex = remainder.indexOf("PUMP=");
        if (pIndex < 0) {
            Serial.println("ERROR_CMD_MISSING_PUMP");
            return;
        }
        String pump = remainder.substring(pIndex + 5, pIndex + 6);
        pump.trim();
        int idx = pump.charAt(0) - 'A';
        if (idx < 0 || idx > 3) {
            Serial.println("ERROR_CMD_INVALID_PUMP");
            return;
        }
        if (remainder.indexOf("STATUS") >= 0) {
            char status[192];
            const char* unitStr = (motors[idx].unit_type == UNIT_UL_MIN) ? "UL/MIN" :
                                  (motors[idx].unit_type == UNIT_UL_HR)  ? "UL/HR"  :
                                  (motors[idx].unit_type == UNIT_ML_MIN) ? "ML/MIN" : "ML/HR";
            const char* gbStr   = (motors[idx].gearbox_type == GEAR_BOX_1_1)   ? "1:1"   :
                                  (motors[idx].gearbox_type == GEAR_BOX_1_25) ? "25:1" : "100:1";
            const char* msStr   = (motors[idx].microstep_type == MICRO_STEP_1_8)  ? "1/8"  :
                                  (motors[idx].microstep_type == MICRO_STEP_1_16) ? "1/16" :
                                  (motors[idx].microstep_type == MICRO_STEP_1_32) ? "1/32" : "1/64";
            const char* rodStr  = (motors[idx].rod_type == ROD_1_STAR) ? "1-START" : "4-START";
            const char* enStr   = (motors[idx].state == RUN) ? "ON" : "OFF";
            snprintf(status, sizeof(status),
                     "PUMP=%c FLOW=%.2f DIAMETER=%.2f DIRECTION=%s STATE=%s UNIT=%s GEARBOX=%s MICROSTEP=%s ROD=%s ENABLE=%s",
                     pump.charAt(0),
                     motors[idx].flow,
                     motors[idx].diameter,
                     motors[idx].direction == RIGHT_DIR ? "INFUSE" : "WITHDRAW",
                     motors[idx].state == RUN ? "RUN" : "STOP", unitStr, gbStr, msStr, rodStr, 
                     motors[idx].enabled ? "ON" : "OFF");
            Serial.println(status);
        } else {
            Serial.println("ERROR_CMD_UNKNOWN_QUERY");
        }
        return;
    } else {
        // Unrecognized command
        Serial.println(RESPONSE_ERROR_CMD);
    }
}
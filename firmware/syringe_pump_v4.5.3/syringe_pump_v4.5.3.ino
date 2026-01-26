/*
 Version of code: V4.5.3 - USB Serial Only
 Version of PCB board: V3.0
 
 KEY FEATURES (v4.5.3):
 - Enhanced input validation with safety limits (flow: 1-6000 µL/h, diameter: 1-50 mm)
 - Real-time error feedback on LCD with immediate retry capability
 - Instant pump operation with saved settings on power-up
 - Improved UI stability with fixed state transitions
 - Correct asterisk indicator across all menu categories
*/

/**
 * State Machine Documentation:
 * 
 * Main States:
 * 0: Main Menu
 * 1: Flow Input
 * 2: Infuse Direction
 * 3: Withdraw Direction
 * 4-28: Motor and Settings
 * 29-32: Motor Enable/Disable
 * 
 * State Transitions:
 * - From Main Menu (0):
 *   - 'A': Go to Flow Input (1)
 *   - 'B': Toggle Direction (2/3)
 *   - 'C': Change Motor (4,26-28)
 *   - '#': Enter Settings Menu (5)
 *   
 * - From Flow Input (1):
 *   - '0-9': Input digits
 *   - '*': Clear input
 *   - '#': Save and return to Main Menu (0)
 * 
 * - From Settings Menu (5-10):
 *   - '2/8': Navigate up/down
 *   - '#': Select/Confirm
 *   - '*': Cancel/Back
 */

// ──────────────────────────────────────────────────────────────
//  SECTION 0 ‒ LIBRARY INCLUDES & CORE DEFINES
// ──────────────────────────────────────────────────────────────
#include "config.h"
#include "serial_commands.h"

// ──────────────────────────────────────────────────────────────
//  SECTION 1 ‒ HARDWARE CONFIGURATION
// ──────────────────────────────────────────────────────────────

// Default Settings
#define FLOW_DEFAULT 		1000        // Default flow rate in uL/h
#define DIAMETER_DEFAULT 	8.17f          // Default syringe diameter in mm
#define GEARBOX_DEFAULT 	GEAR_BOX_1_1
#define ROD_DEFAULT 		ROD_1_STAR
#define MICROSTEP_DEFAULT 	MICRO_STEP_1_8
#define UNIT_DEFAULT 		UNIT_UL_HR
#define DIR_DEFAULT		    RIGHT_DIR
#define STATE_DEFAULT		false

// Safety limits
#define MIN_FLOW_RATE 1     // uL/h
#define MAX_FLOW_RATE 6000  // uL/h
#define MIN_DIAMETER  1.0f   // mm
#define MAX_DIAMETER  50.0f  // mm

// Firmware and Device Information
#define VERSION 			"SP-V4.5.3-USB"
#define DEVICE_ID           "SP‑01"

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
I2CKeyPad         keypad(KEYPAD_ADDR);
PCA9555           ioport(GPIO_ADDR);

// Motor objects
Motor motors[NUM_MOTORS];
// Menu states
MenuStates states[N_STATES];

// Preferences (NVS)
Preferences preferences;

// Stepper objects (ContinuousStepper)
ContinuousStepper<StepperDriver> stepper_a;
ContinuousStepper<StepperDriver> stepper_b;
ContinuousStepper<StepperDriver> stepper_c;
ContinuousStepper<StepperDriver> stepper_d;

// ──────────────────────────────────────────────────────────────
//  MICROSTEPPING CONTROL
// ──────────────────────────────────────────────────────────────

void setMicrostepping(int motor_index, int microstep_value) {
    if (motor_index < 0 || motor_index >= NUM_MOTORS) return;
    
    MicrostepIndex config_index;
    switch(microstep_value) {
        case 8:  config_index = MS_EIGHTH_STEP; break;
        case 16: config_index = MS_SIXTEENTH_STEP; break;
        case 32: config_index = MS_THIRTYSECOND_STEP; break;
        case 64: config_index = MS_SIXTYFOURTH_STEP; break;
        default: config_index = MS_EIGHTH_STEP; break;
    }
    
    const MicrostepConfig& config = MICROSTEP_CONFIGS[config_index];
    ioport.digitalWrite(MS1_PINS[motor_index], config.ms1);
    ioport.digitalWrite(MS2_PINS[motor_index], config.ms2);
    
    #if DEBUG_SERIAL
        Serial.print("Motor "); Serial.print(MOTORS_NAMES[motor_index]);
        Serial.print(" microstepping: 1/"); Serial.print(microstep_value);
        Serial.print(" (MS1:"); Serial.print(config.ms1 ? "H" : "L");
        Serial.print(" MS2:"); Serial.print(config.ms2 ? "H" : "L");
        Serial.println(")");
    #endif
}

// ──────────────────────────────────────────────────────────────
//  SECTION 2 ‒ INITIALIZATION FUNCTIONS
// ──────────────────────────────────────────────────────────────

/**
 * Initialize serial communication and display welcome message
 */
void initSerial() {
    Serial.begin(115200);
    // Wait for serial port to connect (important for native USB)
    while (!Serial && millis() < 2000) {
        ; // Wait for serial port to connect
    }
    #if DEBUG_SERIAL
        Serial.println("\n\n=== Pump Controller Debug Mode ===");
        Serial.print("Firmware: "); Serial.println(VERSION);
        Serial.println("Debug output enabled");
    #endif
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
    
    #if DEBUG_SERIAL
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
    #endif
    
    // Initialize keypad
    if (!keypad.begin()) {
        Serial.println("\nERROR: Cannot communicate with keypad");
        lcd.clear();
        lcd.print("Keypad Error!");
        lcd.setCursor(0, 1);
        lcd.print("Check connection");
        return false;
    }
    
    #if DEBUG_SERIAL
        // Set up keypad interrupt
        Serial.println("Keypad initialized successfully");
        Serial.print("Using interrupt pin: ");
        Serial.println(KEYPAD_INT_PIN);
    #endif

    pinMode(KEYPAD_INT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(KEYPAD_INT_PIN), keyChanged, FALLING);
    key_change = false;
    
    return true;
}

/**
 * Initialize motor presets from NVS or set defaults
 * 
 * IMPORTANT (v4.5.3): This function now calls calculateNewStep() to ensure
 * all motors are ready to run immediately on startup with their last saved settings.
 * Previously, RPM was only calculated when a parameter was changed.
 */
void initMotorPresets() {
    for (int i = 0; i < NUM_MOTORS; i++) {
        // Load saved presets
        loadMotorPresets(i);
        
        // Force all motors to be stopped, enabled, and set to INFUSE direction on reboot
        motors[i].state = STOP;           // Force STOP state
        motors[i].enabled = true;         // Enable the motor
        motors[i].direction = RIGHT_DIR;  // Default to INFUSE direction
        motors[i].initialized = true;
        
        // Calculate RPM based on loaded parameters so pump can run immediately with saved settings (INSTANT STARTUP)
        calculateNewStep(i);
        
        // Save the updated state
        saveMotorPreset(i);
    }
}

/**
 * Initialize motor hardware pins and settings
 */
void initMotorHardware() {
    for (int i = 0; i < NUM_MOTORS; i++) {
        // ASSIGN pins
        motors[i].name = MOTORS_NAMES[i];
        motors[i].step_pin = STEP_PINS[i]; 
        motors[i].dir_pin  = DIR_PINS[i];
        motors[i].en_pin   = EN_PINS[i];

        // Configure direction and step pins
        pinMode(motors[i].dir_pin, OUTPUT);
        pinMode(motors[i].step_pin, OUTPUT);
        digitalWrite(motors[i].dir_pin, RIGHT_DIR); // Set initial direction (INFUSE by default)
        digitalWrite(motors[i].step_pin, LOW); // Ensure motor is stopped

        // Configure enable pins - set LOW to enable the driver (active low)
        ioport.pinMode(motors[i].en_pin, OUTPUT);
        ioport.digitalWrite(motors[i].en_pin, HIGH);  // HIGH = DISABLED
        
        // Configure TMC2209 microstepping pins
        ioport.pinMode(MS1_PINS[i], OUTPUT);
        ioport.pinMode(MS2_PINS[i], OUTPUT);
        
        setMicrostepping(i, motors[i].microstep_type);
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

/**
 * Main setup function
 */
void setup() {
    // Initialize hardware and peripherals
    initSerial();
    initLCD();
    ioport.begin();       
    
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
        Serial.println("WARNING: Operating in serial-only mode");
        lcd.clear();
        lcd.print("Keypad Error!");
        lcd.setCursor(0, 1);
        lcd.print("Using Serial Only");
        delay(2000);
    }
    
    actual_motor = MOTOR_A;  
    num_input = "";

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

// ──────────────────────────────────────────────────────────────
//  SECTION 3 – MAIN LOOP
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
}

// ──────────────────────────────────────────────────────────────
//  SECTION 4 – MOTION CALCS
// ──────────────────────────────────────────────────────────────

void calculateNewStep(int motor_idx)
{
    if (motor_idx < 0) motor_idx = actual_motor; // Default to current selection

    // 1) Grab the raw parameters for the currently selected motor
    int _flow            = motors[motor_idx].flow;           // e.g., "3000" if typed as 3000 μL/h
    float _diameter        = motors[motor_idx].diameter;       // e.g., 8.17 mm
    int   _gearbox         = motors[motor_idx].gearbox_type;   // e.g., 25 for a 1:25 gearbox
    int   _rod             = motors[motor_idx].rod_type;       // Stored as tenths of mm (e.g., 15 for 1.5mm)
    int   _microstep       = motors[motor_idx].microstep_type;  // e.g., 16 for 1/16 microstep
    uint32_t   _unit            = motors[motor_idx].unit_type;       // Stored as integer * 1000 (e.g., 17 for 0.017)in μL/h

    // 2) Convert the desired flow rate to μL/min (if it's not already):
    float unit_scale = _unit / 1000.0f;
    float desired_flow_ul_per_min = _flow * unit_scale;

    // 3) Compute syringe cross‐sectional area (in mm²):
    //    A = π · (D/2)²
    float radius_mm = _diameter / 2.0f;
    float cross_section_mm2 = PI * radius_mm * radius_mm;   // mm²

    // 4) Compute how many μL come out per full revolution of the leadscrew:
    float rod_pitch_mm = _rod / 10.0f;
    float vol_per_rev_ul = cross_section_mm2 * rod_pitch_mm;

    // 5) Now compute the needed RPM (revolutions per minute):
    if (vol_per_rev_ul <= 0.0f || desired_flow_ul_per_min < 0.0f) {
        #if DEBUG_SERIAL
            Serial.println("ERROR: Invalid calculation parameters (vol_per_rev_ul or desired_flow <= 0)");
        #endif
        motors[motor_idx].rpm = 0;  // Safe default - motor stopped
        return;
    }

    float output_rpm = desired_flow_ul_per_min / vol_per_rev_ul;

    // 6) Account for gearbox - motor must spin faster
    float motor_rpm = output_rpm * _gearbox ;

    // 7) Convert RPM to steps per second:
    float steps_per_rev = (float)STEP_PER_REV * _microstep;
    float steps_per_sec = (motor_rpm * steps_per_rev) / 60.0f;

    // 8) Finally, store these back into your motor struct:
    motors[motor_idx].rpm = steps_per_sec;

    // Apply microstepping to hardware
    setMicrostepping(motor_idx, _microstep);

    // 9) Persist the settings to EEPROM (if desired) and print debug info:
    saveMotorPreset(motor_idx);

    #if DEBUG_SERIAL
        Serial.println("\n--- Motion Parameters ---");
        Serial.print("Motor: "); Serial.println(MOTORS_NAMES[motor_idx]);
        Serial.print("Diameter (mm): "); Serial.println(_diameter, 2);
        Serial.print("Raw Flow Value (uL/h): "); Serial.println(_flow);
        Serial.print("Raw Rod Value (mm): "); Serial.println(_rod);
        Serial.print("Gearbox: 1:"); Serial.println(_gearbox);
        Serial.print("Microstep: 1/"); Serial.println(_microstep);

        Serial.println("\n--- Calculations ---");
        Serial.print("Unit scale: "); Serial.println(unit_scale, 4);   
        Serial.print("Flow (uL/min): "); Serial.println(desired_flow_ul_per_min);        
        Serial.print("Rod Pitch (mm): "); Serial.println(rod_pitch_mm, 2);
        Serial.print("Vol/Rev (uL): "); Serial.println(vol_per_rev_ul, 2);
        Serial.print("Output RPM: "); Serial.println(output_rpm, 2);        
        Serial.print("Motor RPM: "); Serial.println(motor_rpm, 2);
        Serial.print("Steps/sec: "); Serial.println(steps_per_sec, 2);

        Serial.println("-------------------------");
    #endif
}

// ---------------- ISR – keypad interrupt ---------------------
static unsigned long lastInterrupt = 0;
void keyChanged(){
    unsigned long now = millis();
    if (now - lastInterrupt > 50) {  // 50ms debounce
        key_change = true;
        lastInterrupt = now;
    }
}
// ──────────────────────────────────────────────────────────────
//  SECTION 5 – PRESET STORAGE & NETWORKING UTILITIES
// ──────────────────────────────────────────────────────────────

// ---------------- saveMotorPreset -----------------
void saveMotorPreset(int index)
{
    String ns = "motor_" + String(MOTORS_NAMES[index]);
    preferences.begin(ns.c_str(), false);  // write mode
    
    preferences.putChar  ("name",   motors[index].name);
    preferences.putUInt  ("flow",   (uint32_t)motors[index].flow);
    preferences.putFloat ("diam",   motors[index].diameter);
    preferences.putUInt  ("gear",   (uint32_t)motors[index].gearbox_type);
    preferences.putUInt  ("rod",    (uint32_t)motors[index].rod_type);
    preferences.putUInt  ("micro",  (uint32_t)motors[index].microstep_type);
    preferences.putUInt  ("unit",   (uint32_t)motors[index].unit_type);
    preferences.putBool  ("dir",    motors[index].direction);
    preferences.putBool  ("state",  motors[index].state);

    preferences.end();
}

// ---------------- loadMotorPresets ----------------
void loadMotorPresets(int index)
{
  String ns = "motor_" + String(MOTORS_NAMES[index]);
  preferences.begin(ns.c_str(), true);

  uint32_t rod = preferences.getUInt("rod", ROD_DEFAULT);
    if (rod == 15) {  // Old value that needs migration
        rod = ROD_DEFAULT;  // Set to 20
        // Save the updated value back to preferences
        preferences.end();
        preferences.begin(ns.c_str(), false);  // Open in read-write mode
        preferences.putUInt("rod", rod);
    }

  motors[index].name            = preferences.getChar  ("name",   MOTORS_NAMES[index]);
  motors[index].flow            = preferences.getUInt  ("flow", FLOW_DEFAULT);
  motors[index].diameter        = preferences.getFloat ("diam", DIAMETER_DEFAULT);
  motors[index].gearbox_type    = preferences.getUInt  ("gear", GEARBOX_DEFAULT);
  //motors[index].rod_type        = preferences.getUInt  ("rod", ROD_DEFAULT);
  motors[index].rod_type        = rod;
  motors[index].microstep_type  = preferences.getUInt  ("micro", MICROSTEP_DEFAULT);
  motors[index].unit_type       = preferences.getUInt  ("unit", UNIT_DEFAULT);
  motors[index].direction       = preferences.getBool  ("dir", DIR_DEFAULT);
  motors[index].state           = preferences.getBool  ("state", STATE_DEFAULT);
  
  preferences.end();

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
// Handles keypad input and manages state transitions
// SAFETY FEATURE (v4.5.3): Input validation on flow (1-6000 µL/h) and diameter (1-50 mm)
// with immediate error feedback. Users can retry without exiting input mode.
void processKey(char _key)
{
  if(_key != 'N')
  {
		// Initialize next_state to current_state to prevent unexpected state changes
		next_state = current_state;
		
		// * (STAR) toggles RUN / STOP
        if(_key == '*' && (current_state!= 1 && current_state != 11))
		{
			motors[actual_motor].state = !motors[actual_motor].state;
			
            if(motors[actual_motor].state) //RUN
			{                
                digitalWrite(motors[actual_motor].dir_pin, motors[actual_motor].direction);
                ioport.digitalWrite(motors[actual_motor].en_pin, LOW);

                #if DEBUG_SERIAL
                Serial.print("Starting Motor ");
                Serial.print(motors[actual_motor].name);
                Serial.print(" at ");
                Serial.print(motors[actual_motor].rpm);
                Serial.println(" steps/sec");
                #endif

				if( actual_motor == MOTOR_A){stepper_a.spin(motors[actual_motor].rpm);}
				if( actual_motor == MOTOR_B){stepper_b.spin(motors[actual_motor].rpm);}
				if( actual_motor == MOTOR_C){stepper_c.spin(motors[actual_motor].rpm);}
				if( actual_motor == MOTOR_D){stepper_d.spin(motors[actual_motor].rpm);}
			}
			else //STOP
			{
				if( actual_motor == MOTOR_A){stepper_a.spin(0); stepper_a.stop();}
				if( actual_motor == MOTOR_B){stepper_b.spin(0); stepper_b.stop();}
				if( actual_motor == MOTOR_C){stepper_c.spin(0); stepper_c.stop();}
				if( actual_motor == MOTOR_D){stepper_d.spin(0); stepper_d.stop();}
                ioport.digitalWrite(motors[actual_motor].en_pin, HIGH);
			}
		}
        if(current_state != 1 && current_state != 11)
        {
            if(_key == 'A')      { next_state = states[current_state].button[3]; } // Flow rate
            else if(_key == 'B') { next_state = states[current_state].button[4]; } // Direction
            else if(_key == 'C') { actual_motor = (actual_motor + 1) % 4; }        // Motor selection
            else if(_key == '2') { next_state = states[current_state].button[0]; } // Up
            else if(_key == '8') { next_state = states[current_state].button[1]; } // Down
            else if(_key == '#') { next_state = states[current_state].button[2]; } // Enter
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
                    motors[actual_motor].flow = 0;  // Set to 0 for flow
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
                    && current_state == 11     // • only on DIAMETER screen
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
			if( (current_state > 12 && current_state < 26)  )
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
                bool flowValid = false;
                
                if (num_input.length() > 0)
                {
                    int flow = (int)num_input.toInt();
                    // Safety validation: Flow must be within 1-6000 µL/h (v4.5.3)
                    if (flow >= MIN_FLOW_RATE && flow <= MAX_FLOW_RATE) {
                        motors[actual_motor].flow = flow;
                        flowValid = true;
                    } else {
                        Serial.println("ERROR: Flow out of range");
                        lcd.setCursor(0, 1);
                        lcd.print("Out of range!   ");
                        delay(1500);
                        lcd.clear();
                        num_input = "";  // Keep in input mode for retry
                        printScreen();
                        return;  // Don't proceed - user can immediately enter a new value
                    }
		        }
				else
                {
                    flowValid = true;
                }
                
                if (flowValid) {
                    calculateNewStep(); // This calls saveMotorPreset internally
                }

                // If motor was running, update its speed immediately
                if (wasRunning) {
                    controlStepperMotor(actual_motor, motors[actual_motor].rpm);
                }
                
                lcd.setCursor(0, 1);
                lcd.print("                ");
                lcd.setCursor(0, 1);
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
                bool diamValid = false;
                
                if (num_input.length() > 0) {
                    float diam = num_input.toFloat();
                    // Safety validation: Diameter must be within 1-50 mm (v4.5.3)
                    if (diam >= MIN_DIAMETER && diam <= MAX_DIAMETER) {
                        motors[actual_motor].diameter = diam;
                        diamValid = true;
                    } else {
                        Serial.println("ERROR: Diameter out of range");
                        lcd.setCursor(0, 1);
                        lcd.print("Out of range!   ");
                        delay(1500);
                        lcd.clear();
                        num_input = "";  // Keep in input mode for retry
                        printScreen();
                        return;  // Don't proceed - user can immediately enter a new value
                    }
                }
				else {
                    diamValid = true;  // No input = keep existing
                }
                
                if (diamValid) {
                    calculateNewStep(); // This calls saveMotorPreset internally
                }

                // If motor was running, update its speed immediately
                if (wasRunning) {
                    controlStepperMotor(actual_motor, motors[actual_motor].rpm);
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

		if(current_state > 11 && current_state < 25)
		{
			if( current_state == 13 && motors[actual_motor].unit_type == UNIT_UL_MIN) lcd.print(" *");
			if( current_state == 14 && motors[actual_motor].unit_type == UNIT_UL_HR) lcd.print(" *");
			if( current_state == 15 && motors[actual_motor].unit_type == UNIT_ML_MIN) lcd.print(" *");
			if( current_state == 16 && motors[actual_motor].unit_type == UNIT_ML_HR) lcd.print(" *");

			if( current_state == 17 && motors[actual_motor].gearbox_type == GEAR_BOX_1_1) lcd.print(" *");
			if( current_state == 18 && motors[actual_motor].gearbox_type == GEAR_BOX_1_25) lcd.print(" *");
			if( current_state == 19 && motors[actual_motor].gearbox_type == GEAR_BOX_1_100) lcd.print(" *");

			if( current_state == 20 && motors[actual_motor].microstep_type == MICRO_STEP_1_8) lcd.print(" *");
			if( current_state == 21 && motors[actual_motor].microstep_type == MICRO_STEP_1_16) lcd.print(" *");
			if( current_state == 22 && motors[actual_motor].microstep_type == MICRO_STEP_1_32) lcd.print(" *");
			if( current_state == 23 && motors[actual_motor].microstep_type == MICRO_STEP_1_64) lcd.print(" *");

			if( current_state == 24 && motors[actual_motor].rod_type == ROD_1_STAR) lcd.print(" *");
			if( current_state == 25 && motors[actual_motor].rod_type == ROD_4_STAR) lcd.print(" *");
		}

		// show next-item on second row
    	if (current_state < N_STATES)
    	{
			uint8_t nextIdx = states[current_state].button[1];
            if (nextIdx < N_STATES) {
                lcd.setCursor(0,1);
                lcd.print("  ");
                lcd.print(states[nextIdx].text);
            }
			
			if(current_state > 11 && current_state < 25)
			{
				if( current_state == 13 && motors[actual_motor].unit_type == UNIT_UL_HR) lcd.print(" *");
				if( current_state == 14 && motors[actual_motor].unit_type == UNIT_ML_MIN) lcd.print(" *");
				if( current_state == 15 && motors[actual_motor].unit_type == UNIT_ML_HR) lcd.print(" *");
				if( current_state == 16 && motors[actual_motor].unit_type == UNIT_UL_MIN) lcd.print(" *");

				if( current_state == 19 && motors[actual_motor].gearbox_type == GEAR_BOX_1_1) lcd.print(" *");
				if( current_state == 17 && motors[actual_motor].gearbox_type == GEAR_BOX_1_25) lcd.print(" *");
				if( current_state == 18 && motors[actual_motor].gearbox_type == GEAR_BOX_1_100) lcd.print(" *");

				if( current_state == 23 && motors[actual_motor].microstep_type == MICRO_STEP_1_8) lcd.print(" *");
				if( current_state == 20 && motors[actual_motor].microstep_type == MICRO_STEP_1_16) lcd.print(" *");
				if( current_state == 21 && motors[actual_motor].microstep_type == MICRO_STEP_1_32) lcd.print(" *");
				if( current_state == 22 && motors[actual_motor].microstep_type == MICRO_STEP_1_64) lcd.print(" *");

				if( current_state == 24 && motors[actual_motor].rod_type == ROD_4_STAR) lcd.print(" *");
				if( current_state == 25 && motors[actual_motor].rod_type == ROD_1_STAR) lcd.print(" *");
			}
		}
	}
	
	if (states[current_state].type == LIST_FILL)
	{
		lcd.clear();
		lcd.setCursor(15,0); lcd.print(motors[actual_motor].name);
		lcd.setCursor(0,0);  lcd.print("> ");
		lcd.print(states[current_state].text);

		if(current_state == 2 && motors[actual_motor].direction == RIGHT_DIR ) lcd.print(" *");
		if(current_state == 3 && motors[actual_motor].direction == LEFT_DIR ) lcd.print(" *");

		lcd.setCursor(0,1);
		lcd.print("  ");
		lcd.print(states[states[current_state].button[4]].text);

		if(current_state == 3 && motors[actual_motor].direction == RIGHT_DIR ) lcd.print(" *");
		if(current_state == 2 && motors[actual_motor].direction == LEFT_DIR ) lcd.print(" *");
	}
}
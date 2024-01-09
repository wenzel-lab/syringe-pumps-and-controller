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
#define LCD_SDA_PIN 4
#define LCD_SCL_PIN 5
// Onboard LED Pin
#define LED_PIN 25
// Keypad Pins
const int pinRows[4] = {15, 14, 13, 12};
const int pinCols[4] = {11, 10, 9, 8};

// Pump Pins (A, B, C)
// Each pump has power, direction, step, RX, TX, and enable pins
// SoftwareSerial is used for serial communication with the pumps
#define PUMP_A_POWER 16
#define PUMP_A_DIR_PIN 17
#define PUMP_A_STEP_PIN 18
#define PUMP_A_RX_PIN 19
#define PUMP_A_TX_PIN 20
#define PUMP_A_EN_PIN 21
SoftwareSerial serialPumpA(PUMP_A_TX_PIN, PUMP_A_RX_PIN);  // RX, TX for Pump A
#define PUMP_A_SERIAL_PORT serialPumpA

// Similar definitions for PUMP_B and PUMP_C ...

#define DRIVER_ADDRESS 0b00  // TMC2209 Driver address (MS1 and MS2 configuration)
#define R_SENSE 0.1f         // Current sensing resistance, match to your driver specs

// Motor driver setup for each pump
TMC2209Stepper driverPumpA(&PUMP_A_SERIAL_PORT, R_SENSE, DRIVER_ADDRESS);
TMC2209Stepper driverPumpB(&PUMP_B_SERIAL_PORT, R_SENSE, DRIVER_ADDRESS);
TMC2209Stepper driverPumpC(&PUMP_C_SERIAL_PORT, R_SENSE, DRIVER_ADDRESS);
TMC2209Stepper* driver[3] = {&driverPumpA, &driverPumpB, &driverPumpC};

/**********************************************************************
 * VARIABLES
 **********************************************************************/
// Keypad configuration
char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
int keysPressed[4][4] = {
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0}
};

// Structure to store keypad inputs
struct KeypadInput {
    char key;
    int clickType;
};

// Pump structure definition
struct Pump {
    // Pump properties and state variables
    bool isSelected;
    float diameter;
    bool start;
    int flowRate;
    bool activate;
    int direction;
    int posLCD;
    String name;
    int stepState;
    long timePump;
    int rms;
    // Pins and serial port for each pump
    int pumpPowerPin;
    int pumpDirPin;
    int pumpStepPin;
    int pumpRxPin;
    int pumpTxPin;
    int pumpEnPin;
    SoftwareSerial* pumpSerialPort;
};

// Pump object instances
Pump pumpA, pumpB, pumpC;
Pump* pump[3];  // Array of pointers to pump objects

// Menu and control variables
int mode = 1;  // Flow mode (1) or diameter mode (0)
bool lcdInputActive = 0;
String lcdInput = "";
int led = 0;

// Pumping calculations
float stepsPerMm = 0.89109589;
int microSteps = 256;  // Microstep settings for TMC2209 (8, 16, 32, 64, 256 supported)

// Setup function
void setup() {
    // Initial setup code...
}

// Main loop
void loop() {
    // Main loop code...
}

// Additional functions for setup, loop, and other operations...
// Setup function
void setup() {
    Serial.begin(9600);  // Start serial communication at 9600 baud rate

    // Configure onboard LED pin as output
    pinMode(LED_PIN, OUTPUT);

    // Set up keypad pins
    for (int i = 0; i < 4; i++) {
        pinMode(pinCols[i], OUTPUT);
        pinMode(pinRows[i], INPUT_PULLDOWN); // Pull-down is important for stable readings
    }

    // Initialize LCD
    lcd.init();
    lcd.backlight();

    // Initialize pumps with their properties
    // Pump A initialization
    pumpA = {
        false,          // isSelected
        4.78,           // diameter
        false,          // start
        1200,           // flowRate
        true,           // activate
        1,              // direction
        2,              // posLCD
        "A",            // name
        0,              // stepState
        0,              // timePump
        1000,           // rms
        PUMP_A_POWER,   // pumpPowerPin
        PUMP_A_DIR_PIN, // pumpDirPin
        PUMP_A_STEP_PIN,// pumpStepPin
        PUMP_A_RX_PIN,  // pumpRxPin
        PUMP_A_TX_PIN,  // pumpTxPin
        PUMP_A_EN_PIN,  // pumpEnPin
        &PUMP_A_SERIAL_PORT // pumpSerialPort
    };
    pump[0] = &pumpA;

    // Similar initializations for pumpB and pumpC ...

    // Other setup code...
}

// Main loop function
void loop() {
    // Blink onboard LED
    digitalWrite(LED_PIN, led); 
    led = !led;

    // Read and process keypad input
    KeypadInput key = readKey();
    mainMenu(key);  // Handle main menu interaction based on the key input
}

// Additional setup function (setup1)
void setup1() {
    // Additional setup tasks...
}

// Additional loop function (loop1)
void loop1() {
    // Additional loop tasks...
}

// Main menu handling function
void mainMenu(KeypadInput &key) {
    // Logic to handle main menu interactions
    // Code handling different keypad inputs for navigation and selection
}

// Function to read keypad input
KeypadInput readKey() {
    // Logic to read and process keypad inputs
    // This function scans the keypad and returns the pressed key and its type (e.g., single click, long press)
}

// Function to update the LCD display
bool updateLCD() {
    // Logic to update LCD display with the current status of pumps and other information
}

// Function to show pump status on LCD
void showPump(Pump &pump) {
    // Logic to display individual pump status on the LCD
}

// Function to calculate the time for each step based on flow rate and diameter
int calculateStepTime(int flow, float diameter) {
    // Calculation logic for step timing
}

// Function to control pump movement
void movePump() {
    // Logic to control the movement of the pump(s) based on current settings
}

// End of the code
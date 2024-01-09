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

#define DRIVER_ADDRESS 0b00  // TMC2209 and TMC2226 Driver address (MS1 and MS2 configuration)
#define R_SENSE 0.1f         // Current sensing resistance, match to your driver specs

// Motor driver setup for each pump - we use the driver module TMC2226 but the supplier software is still releaased for TMC2209.
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
int microSteps = 256;  // Microstep settings for TMC2209 and TMC2226 (8, 16, 32, 64, 256 supported)

// Setup function
void setup() {
    Serial.begin(9600);

    //inbuilt led pin setup
    pinMode(LED_PIN, OUTPUT);

    //keypad pins setup 
    for(int i=0; i<4; i++) {
        pinMode(pinCols[i], OUTPUT);
        pinMode(pinRows[i], INPUT_PULLDOWN); // pulldown important
    };

    //initialize LCD
    lcd.init();
    lcd.backlight();    
    //initialize pumps
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
    pumpB = {0,4.78,0,1500,1,0,7,"B",0,0,1500,
    PUMP_B_POWER,
    PUMP_B_DIR_PIN,
    PUMP_B_STEP_PIN,
    PUMP_B_RX_PIN,
    PUMP_B_TX_PIN,
    PUMP_B_EN_PIN,
    &PUMP_B_SERIAL_PORT};
    pump[1] = &pumpB;
    pumpC = {0,4.78,0,4030,0,1,12,"C",0,0,1000,
    PUMP_C_POWER,
    PUMP_C_DIR_PIN,
    PUMP_C_STEP_PIN,
    PUMP_C_RX_PIN,
    PUMP_C_TX_PIN,
    PUMP_C_EN_PIN,
    &PUMP_C_SERIAL_PORT};
    pump[2] = &pumpC;
}

// Main loop function
void loop() {
    // Blink onboard LED
    digitalWrite(LED_PIN, led); led = !led;

    // Read and process keypad input
    KeypadInput key = readKey();
    //Serial.println(String(key.key)+" "+String(key.clickType));
    //delay(100);
    mainMenu(key); // Handle main menu interaction based on the key input  
}

// Additional functions for setup, loop, and other operations...
// Setup function
void setup1() {
    Serial.begin(9600);
    while(!pump[0]){};
    delay(2000); // wait for pumps init

    for (int i=0;i<2;i++){
        pinMode(pump[i]->pumpPowerPin, OUTPUT);
        pinMode(pump[i]->pumpDirPin, OUTPUT);
        pinMode(pump[i]->pumpStepPin, OUTPUT);
        pinMode(pump[i]->pumpEnPin, OUTPUT);

        digitalWrite(pump[i]->pumpPowerPin, 1);
        Serial.println(pump[i]->pumpPowerPin);
        digitalWrite(pump[i]->pumpEnPin, 0);

        pump[i]->pumpSerialPort->begin(115200);

        driver[i]->begin();                 // Enables driver in software
        driver[i]->toff(5);                 // time between turning off the current to one coil and turning on to the other coil
        driver[i]->rms_current(pump[i]->rms);        // Set motor RMS current
        driver[i]->microsteps(microSteps);         // Set microsteps
        driver[i]->pwm_autoscale(true);     // Needed for stealthChop
        driver[i]->shaft(pump[i]->direction);   
        updateLCD();
    }
}

// Additional loop function (loop1)
void loop1() {
    movePump();
}

// Main menu handling function
void mainMenu(KeypadInput &key){
    // Logic to handle main menu interactions
    // Code handling different keypad inputs for navigation and selection
    int n;
    String order = "ABC";
    n = order.indexOf(key.key);
    if (n != -1){
        // es pump
        if (key.clickType == 1){
            lcdInputActive = 0;
            lcdInput = "";
            pump[n]->isSelected = !pump[n]->isSelected;
            for (int i=0;i<3;i++){if(i!=n){pump[i]->isSelected=0;};};
            updateLCD();
        }else if (key.clickType == 2){
            //desactivar           
            pump[n]->activate = !pump[n]->activate;
            updateLCD();
            delay(1000);
        }
    }else{
        int n;
        String order = "*#";
        n = order.indexOf(key.key);
        if (n != -1){
            // es simbolo o D;
            bool pumpSelected = 0;
            for (int i=0;i<3;i++){
                if (pump[i]->isSelected){
                    pumpSelected += 1;
                    if (key.key=='#' && lcdInputActive){
                        lcdInputActive = 0;
                        if(mode){pump[i]->flowRate = atoi(lcdInput.c_str());}
                        else{pump[i]->diameter = atof(lcdInput.c_str());}
                        
                        lcdInput = "";
                        pump[i]->isSelected = 0;
                        updateLCD();
                    }else if (key.key == '*' && !lcdInputActive){
                        pump[i]->start = !pump[i]->start;
                        pump[i]->isSelected = 0;
                        updateLCD();
                    }
                }
            }
            if (!pumpSelected){
                if(key.key == '#' ){
                    //cambiar modo
                    mode =!mode;
                    updateLCD();
                }
            }
        }
        order = "1234567890D";
        n = order.indexOf(key.key);
        if (n != -1){
            // es numero
            lcdInputActive = 1;
            if (key.key=='D'){lcdInput += String('.');}
            else{lcdInput += String(key.key);}
            updateLCD();
        }
    }
}

// Function to read keypad input
KeypadInput readKey(){
    // Logic to read and process keypad inputs
    // This function scans the keypad and returns the pressed key and its type (e.g., single click, long press)
    KeypadInput input = {' ', 0};    
        for (int c=0;c<4;c++){
            digitalWrite(pinCols[c], 1);
            for (int r=0;r<4;r++){
                if (digitalRead(pinRows[r])==HIGH){
                    // wait for realease and reset timecount
                    long m = millis();
                    while (digitalRead(pinRows[r])==HIGH){
                        if (millis()-m > 1000){
                            keysPresed[r][c] += 1;
                            break;
                        }
                    }
                    keysPresed[r][c] += 1;
                    Serial.println(keys[r][c]);
                    if (input.clickType < keysPresed[r][c]){
                        input = {keys[r][c], keysPresed[r][c]};
                    }
                }
            }
            digitalWrite(pinCols[c], 0);
        }
    // clean keys
    for (int c=0;c<4;c++){
        for (int r=0;r<4;r++){
            keysPresed[r][c]=0;
        }
    }
    return input;
}

// Function to update the LCD display
bool updateLCD(){
    // Logic to update LCD display with the current status of pumps and other information
    lcd.clear();
    lcd.setCursor(0, 1);
    if (mode){lcd.print("F:");}else{lcd.print("D:");};
    showPump(pumpA);
    showPump(pumpB);
    showPump(pumpC);
    return true;
}

// Function to show pump status on LCD
void showPump(Pump &pump){
    // Logic to display individual pump status on the LCD
    if (pump.activate){
        if (pump.isSelected){
            lcd.setCursor(pump.posLCD, 0);
            if (pump.start){
                lcd.print('*'+pump.name+">");
            }else{
                lcd.print('*'+pump.name);
            }
            lcd.setCursor(pump.posLCD,1);
            if (lcdInputActive){
                lcd.print(lcdInput);
            }else {
                if(mode){lcd.print(String(pump.flowRate));}
                else{lcd.print(String(pump.diameter));}
                
            }
        }else{
            lcd.setCursor(pump.posLCD, 0);
            if (pump.start){lcd.print(pump.name+">");}else{lcd.print(pump.name);};
            lcd.setCursor(pump.posLCD,1);
            if(mode){lcd.print(String(pump.flowRate));}
            else{lcd.print(String(pump.diameter));}
        }
        
    }
}

// Function to calculate the time for each step based on flow rate and diameter
int calculateStepTime(int flow, float diameter) {
    // Calculation logic for step timing
    float a = 3.1415*pow(diameter/2,2);
    float speed = ((((float)flow/(float)3600)/(float)a)/(float)1000000)*(float)stepsPerMm;  //en mm3/ms
    int t = (float)1/speed;
    t = t/microSteps;
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
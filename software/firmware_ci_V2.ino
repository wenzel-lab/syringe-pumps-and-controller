#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
#include <TMCStepper.h>
#include <SoftwareSerial.h>

/**********************************************************************
 * PINS     ##remember they corresponds to the GP pins
**********************************************************************/
//LCD
#define LCD_SDA_PIN 4
#define LCD_SCL_PIN 5
//INBOARD LED
#define LED_PIN 25
//KEYPAD
const int pinRows[4]={15,14,13,12};
const int pinCols[4]={11,10,9,8};
//PUMPS
#define PUMP_A_POWER 16
#define PUMP_A_DIR_PIN 17
#define PUMP_A_STEP_PIN 18
#define PUMP_A_RX_PIN 19
#define PUMP_A_TX_PIN 20
#define PUMP_A_EN_PIN 21
SoftwareSerial serialPumpA(PUMP_A_TX_PIN,PUMP_A_RX_PIN); // RX, TX
#define PUMP_A_SERIAL_PORT serialPumpA

#define PUMP_B_POWER 22
#define PUMP_B_DIR_PIN 26
#define PUMP_B_STEP_PIN 27
#define PUMP_B_RX_PIN 28
#define PUMP_B_TX_PIN 7
#define PUMP_B_EN_PIN 6
SoftwareSerial serialPumpB(PUMP_B_TX_PIN,PUMP_B_RX_PIN); // RX, TX
#define PUMP_B_SERIAL_PORT serialPumpB

#define DRIVER_ADDRESS 0b00 // TMC2209 Driver address according to MS1 and MS2
#define R_SENSE 0.1f // Match to your driver

TMC2209Stepper driverPumpA(&PUMP_A_SERIAL_PORT, R_SENSE, DRIVER_ADDRESS);
TMC2209Stepper driverPumpB(&PUMP_B_SERIAL_PORT, R_SENSE, DRIVER_ADDRESS);

TMC2209Stepper* driver[2] = {&driverPumpA,&driverPumpB};

/**********************************************************************
 * VARIABLES
**********************************************************************/
//KEYPAD
char keys[4][4]={
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
int keysPresed[4][4]={
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0}
};
struct KeypadInput {
    char key;
    int clickType;
};
//PUMPS
struct Pump {
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
    int pumpPowerPin;
    int pumpDirPin; 
    int pumpStepPin;
    int pumpRxPin;
    int pumpTxPin;
    int pumpEnPin;
    SoftwareSerial* pumpSerialPort;
};
Pump pumpA;
Pump pumpB;

Pump* pump[2];
//MENU
int mode = 1; // 1 for flow, 0 for diamater.
bool lcdInputActive = 0;
String lcdInput = "";
int led = 0;

//FOR PUMPING
// 0.89109589 (4-start rod, leadscrew pitch: 8 mm)
float stepsPerMm = 4562.4109568; // (1-start rod, leadscrew pitch: 2.5 mm included a 100:1 gearbox) 

// TMC2209 only supports 8, 16, 32, 64, 256 // Original: 256
int microSteps = 16;

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
    pumpA = {0,8.17,0,1000,1,1,2,"A",0,0,1200,
    PUMP_A_POWER,
    PUMP_A_DIR_PIN,
    PUMP_A_STEP_PIN,
    PUMP_A_RX_PIN,
    PUMP_A_TX_PIN,
    PUMP_A_EN_PIN,
    &PUMP_A_SERIAL_PORT};
    pump[0] = &pumpA;
    pumpB = {0,4.78,0,1000,1,0,7,"B",0,0,1200,
    PUMP_B_POWER,
    PUMP_B_DIR_PIN,
    PUMP_B_STEP_PIN,
    PUMP_B_RX_PIN,
    PUMP_B_TX_PIN,
    PUMP_B_EN_PIN,
    &PUMP_B_SERIAL_PORT};
    pump[1] = &pumpB;
}

void loop() {
    digitalWrite(LED_PIN, led); led = !led;
    KeypadInput key = readKey();
    //Serial.println(String(key.key)+" "+String(key.clickType));
    //delay(100);
    mainMenu(key);   
}

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

void loop1() {
    movePump();
}

void mainMenu(KeypadInput &key){
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
            for (int i=0;i<2;i++){
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

KeypadInput readKey(){
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

bool updateLCD(){
    lcd.clear();
    lcd.setCursor(0, 1);
    if (mode){lcd.print("F:");}else{lcd.print("D:");};
    showPump(pumpA);
    showPump(pumpB);
    return true;
}

void showPump(Pump &pump){
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

int calculateStepTime(int flow, float diameter) {
    float a = 3.1415*pow(diameter/2,2);
    float speed = ((((float)flow/(float)3600)/(float)a)/(float)1000000)*(float)stepsPerMm;  //en mm3/ms
    int t = (float)1/speed;
    t = t/microSteps;
    return t;
}

void movePump() {
    for (int i=0;i<2;i++){
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

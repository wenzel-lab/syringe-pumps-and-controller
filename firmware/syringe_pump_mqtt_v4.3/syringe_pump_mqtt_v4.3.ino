/*
 Author: WAC @ LibreHub & Pierre Padilla @ LibreHub

 Version of code: V4.3

	Features:
	--------
	- <*> Clear input buffer (during diameter or flow input)
	- 'Withdraw' option works as intended
	- Translation of setting list to English
	- Keypad description
	- Update of library list

	Past features (V.2):
	--------
	- New flowchart
	- GUI updated
	- Steppers motor are controlled by using the rpm.
	- Save setup to flash with preference.h
	- Add comments
	- Add mqtt connections
	- Add matt commands

	Libraries (Arduino IDE 2.3.5):
	---------
	- PCA9555									    / Author: Nico Verduin / Source: https://github.com/nicoverduin/PCA9555 
	- PCF8574 library     @ 2.3.7 / Author: Renzo Mischianti   #GPIO extensor
	- I2CKeyPad           @ 0.5.0 / Author: Rob Tillaart       #Keypad
	- LiquidCrystal I2C 	@ 1.1.2 / Author: Frank de Brabander #LCD I2C display
	- ArduinoJson					@ 7.4.1 / Author: Benoit Blanchon    #JSON
	- PubSubClient 				@ 2.8		/ Author: Nick O'Leary		   # MQTT client
	- ContinuousStepper 	@ 3.1.0 / Author: Benoit Blanchon		 # Control of stepper motors by rpm

	Stepper Connection:
	-------------------
	Red    A+
	Green  A-
	Black  B+
	Blue   B-

  KEYPAD:
  ------
  <A> : enter flow‐input screen
  <B> : toggle infuse/withdraw mode
  <C> : select (cycle) active pump (A → B → C → D)
  <D> : insert decimal point when typing flow or diameter
  <*> : start/stop the pump (when on main screen), or clear the current input buffer (when typing)
  <#> : enter settings menu (from main), or confirm highlighted option or numeric entry
  <2> : move cursor up in settings lists
  <8> : move cursor down in settings lists

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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <ContinuousStepper.h>
#include <Preferences.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ──────────────────────────────────────────────────────────────
//  SECTION 1 ‒ CONSTANTS, MACROS, GLOBAL ARRAYS
// ──────────────────────────────────────────────────────────────

// LCD display
#define LCD_ADDR 0x27        
// Keypad (I2CKeyPad) address
#define KEYPAD_ADDR 0x24  	
// IO Expander address
#define GPIO_ADDR 0x20 		 

// Stepper pins
int  EN_PINS[]   		= {15, 11,  7,  3};
int  STEP_PINS[] 		= { 4, 17, 26, 33};
int  DIR_PINS[]  		= {16, 18, 25, 32};
char MOTORS_NAMES[] = {'A','B','C','D'};
constexpr uint8_t MS1_PIN[] = { 14,  10,  6,  2 };
constexpr uint8_t MS2_PIN[] = { 13,   9,  5,  1 };

#define INTERNET_CONTROL false

#define RUN 1
#define STOP 0

#define RIGHT_DIR 1
#define LEFT_DIR 0

#define MOTOR_A 0
#define MOTOR_B 1
#define MOTOR_C 2
#define MOTOR_D 3

#define GEAR_BOX_1_1 	    1
#define GEAR_BOX_1_25 	 25
#define GEAR_BOX_1_100 	100

#define ROD_1_STAR 	      2.5 //mm
#define ROD_4_STAR 	      8 //mm

#define MICRO_STEP_1_8 		8 	
#define MICRO_STEP_1_16 	16  
#define MICRO_STEP_1_32 	32  
#define MICRO_STEP_1_64   64  

#define UNIT_UL_MIN			1.0f        				// if user typed “flow” in μL/min
#define UNIT_UL_HR 			(1.0f / 60.0f)  		// if user typed “flow” in μL/h
#define UNIT_ML_MIN 		1000.0f    				  // if user typed “flow” in mL/min
#define UNIT_ML_HR 			(1000.0f / 60.0f)  	// if typed mL/h

#define LEAD 						2.5 // mm/rev
#define STEP_PER_REV  	200 //(200 step/rev)
#define PI 3.14159265

#define IDLE 0
#define SETTINGS 1

#define N_STATES 34
#define NUM_MOTORS 4

#define STEPS_DEFAULT 		  1000
#define RPM_DEFAULT 		    100.0
#define FLOW_DEFAULT 		    1000.0 // uL/h
#define DIAMETER_DEFAULT 	  8.17 // inner diameter of 3 mL syringe
#define GEAR_BOX_DEFAULT  	GEAR_BOX_1_1 // no gearbox
#define MICROSTEP_DEFAULT   MICRO_STEP_1_16
#define UNIT_TYPE_DEFAULT   UNIT_UL_HR
#define ROD_DEFAULT 		    ROD_1_STAR // leadscrew pitch: 2.5 mm
#define DIR_DEFAULT			    RIGHT_DIR  // infuse mode
#define STATE_DEFAULT		    false

#define ID 										"SP-01"
#define VERSION  							"SP-V4.3"

#define RESPONSE_OK     			"OK"
#define RESPONSE_ERROR  			"ERROR"
#define RESPONSE_ERROR_JSON  	"ERROR_INPUT_JSON"
#define RESPONSE_ERROR_CMD 		"ERROR_CMD"

#define WIFI_TIMEOUT 60
#define MQTT_TIMEOUT 5

// MQTT topics / credentials
#define PUBLISH_TOPIC  	"sp/tx"
#define SUBSCRIBE_TOPIC "sp/rx"
#define mqtt_server 	  "35.223.234.244"
#define mqtt_user 		  "iowlabs"
#define mqtt_password 	"!iow_woi!"
#define mqtt_port       1883

// Wi-Fi credentials
const char* ssid = "Taller.1";
const char* password = "@Salvo20";

// List-item enum for the menu system
enum listOfItemTypes {ACTIVITY,TEXT, LIST, DATA, LIST_MOTOR,LIST_FILL};

// ──────────────────────────────────────────────────────────────
//  SECTION 2 ‒ OBJECT INSTANTIATION
// ──────────────────────────────────────────────────────────────

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
I2CKeyPad         keypad(KEYPAD_ADDR);
PCA9555           ioport(GPIO_ADDR);

// FreeRTOS task handles
TaskHandle_t TaskMotors;
TaskHandle_t Task1;

// Preferences (NVS)
Preferences preferences;

// Stepper objects (ContinuousStepper)
ContinuousStepper<StepperDriver> stepper_a;
ContinuousStepper<StepperDriver> stepper_b;
ContinuousStepper<StepperDriver> stepper_c;
ContinuousStepper<StepperDriver> stepper_d;

// Networking
WiFiClient   wifiClient;
PubSubClient client(wifiClient);

// ──────────────────────────────────────────────────────────────
//  SECTION 3 ‒ STRUCTS, GLOBAL VARIABLES & STATE ARRAYS
// ──────────────────────────────────────────────────────────────

struct MenuStates
{
	uint8_t button[5];
	uint8_t type;
	char *text;
};

struct Motor
{
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
	bool direction;
	bool state;
};

uint32_t start, stop;
uint32_t lastKeyPressed = 0;
bool key_change = false;

const char keyMap[16] = { '1','2','3','A',
                          '4','5','6','B',
                          '7','8','9','C',
                          '*','0','#','D' };

MenuStates states[N_STATES];
Motor motors[NUM_MOTORS];

uint8_t next_state = 0;
uint8_t current_state = 0;
uint8_t	estado_pantalla = IDLE;
uint8_t estado_motor = STOP;
char    actual_motor = MOTOR_A;
char    key;
String  num_input = "";

//cmd
const char* cmd;
const char* rcvd_id;
const char* response = RESPONSE_OK;

// connection
bool wifi_status = false;
int wifi_try = 0;
int mqtt_try = 0;
bool bussy_mqtt = false;

// ──────────────────────────────────────────────────────────────
//  SECTION 4 ‒ FORWARD DECLARATIONS
// ──────────────────────────────────────────────────────────────

char getKey();
void calculateNewStep();
void moveStepper(int _steps, int _ch);
void moveMotors();
void processKey(char _key);
void keyChanged();
void printScreen();
void saveMotorPreset(int index);
void loadMotorPresets(int index);
void setup_wifi();
void reconnect();
void processCmd(byte* payload, unsigned int length);
void publishMqtt(char *payload);
void publishResponse();
void callback(char* topic, byte* payload, unsigned int length);
void Task1code(void *pvParameters);
void TaskRunMotors( void * pvParameters );

void setMicrostep(uint8_t motor, uint16_t microstep);

// ──────────────────────────────────────────────────────────────
//  SECTION 5 ‒ FULL SETUP()
// ──────────────────────────────────────────────────────────────

void setup()
{
	Serial.begin(9600);

	Serial.println("\n ----------------------------\n");
	Serial.println(" Syringe pump controller firmware");
	Serial.print(" Version code : ");
	Serial.println(VERSION);
	Serial.println("\n ----------------------------\n");

  Wire.begin();
  lcd.init();
	lcd.backlight();
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Wenzel - Lab ");
	lcd.setCursor(0, 1);
	lcd.print("Syringe Pump");
	delay(3000);
 	if (keypad.begin() == false)
  	{
    	Serial.println("\nERROR: cannot communicate to keypad.\nPlease reboot.\n");
    	while(1);
  	}

	// NOTE: PCF8574 will generate an interrupt on key press and release.
	pinMode(5, INPUT_PULLUP);
	attachInterrupt(5, keyChanged, FALLING); // only release
	key_change = false;

	ioport.begin();

	// Load presets from NVS
	for (size_t i = 0; i < NUM_MOTORS; i++)
	{
		loadMotorPresets(i);
	}
	
	//Define pin modes
	for(int i = 0; i < NUM_MOTORS; i++)
	{
		ioport.pinMode(EN_PINS[i], OUTPUT);
		ioport.digitalWrite(EN_PINS[i], LOW);

		pinMode(DIR_PINS[i],OUTPUT);
		pinMode(STEP_PINS[i],OUTPUT);

		ioport.pinMode(MS1_PIN[i], OUTPUT);
  	ioport.pinMode(MS2_PIN[i], OUTPUT);

		setMicrostep(i, motors[i].microstep_type); 
		//setMicrostep(i, 1);             // 1 = full-step
  	//motors[i].microstep_type = 1;   // keep math in sync
	}

  // --------------- STATE-MACHINE TABLE -------------------------
	// 			  	UP  DOWN 	ENTER 	Flow 	DIR
	states[0]  = {0, 	0, 		5,		1,		2, ACTIVITY, "MOTOR"};

	states[1]  = {1,   1, 	0,		1,	 	1, DATA, "Flow"}; 					// Flow input

	states[2]  = {2, 	2, 		0,		1,		3, LIST_FILL, "Infuse"};		// Infuse/Withdraw menu
	states[3]  = {3, 	3, 		0,		1,		2, LIST_FILL, "Withdraw"};	

	states[4]   = {4, 	4, 		0,		1,		4,  LIST_MOTOR, "A"}; 		// Motor-select menu
	states[26]  = {26, 	26,		0,		1,		26, LIST_MOTOR, "B"}; 
	states[27]  = {27, 	27,		0,		1,		27, LIST_MOTOR, "C"}; 
	states[28]  = {28, 	28,		0,		1,		28, LIST_MOTOR, "D"}; 

	// Settings submenu (Diameter → Units → Gearbox → Microstep → Leadscrew → Enable list)
	states[5]  = {10, 	6,	   11,		1,		2,  LIST, "Diameter"};	// SETTINGS MS0
	states[6]  = {5, 	7,     13,		1,		2,  LIST, "Unit"};				// SETTINGS MS1
	states[7]  = {6, 	8, 	   17,		1,		2,  LIST, "Gearbox"};			// SETTINGS MS2
	states[8]  = {7, 	9, 	   20,		1,		2,  LIST, "Microstep"};		// SETTINGS MS3
	states[9]  = {8, 	10,	   24,		1,		2,  LIST, "Thread Rod"}; 	// SETTINGS MS4
	states[10] = {9, 	5, 	   29,		1,		2,  LIST, "Enable"}; 			// SETTINGS MS4

	// Diameter input & confirmation
	states[11] = {11, 	11, 	0,		1,		2, DATA, "Diameter"}; 
	states[12] = {12, 	12, 	0,		1,		2, DATA, "Diameter OK"};

	// Unit list
	states[13]  = {16, 	14, 	0,		1,		2, LIST, "ul/min"}; 	
	states[14]  = {13, 	15, 	0,		1,		2, LIST, "ul/h"}; 	
	states[15]  = {14, 	16,   0,		1,		2, LIST, "ml/min"}; 	
	states[16]  = {15, 	13, 	0,		1,		2, LIST, "ml/h"}; 	

	// Gear-box list
	states[17] = {19, 	18, 	0,		1,		2, LIST, "1:1"}; 	
	states[18] = {17, 	19, 	0,		1,		2, LIST, "25:1"};	
	states[19] = {18, 	17, 	0,		1,		2, LIST, "100:1"};	

	// Microstep list
	states[20] = {23, 	21, 	0,		1,		2, LIST, "1/8"}; 	
	states[21] = {20, 	22, 	0,		1,		2, LIST, "1/16"};	
	states[22] = {21, 	23, 	0,		1,		2, LIST, "1/32"};	
	states[23] = {22, 	20, 	0,		1,		2, LIST, "1/64"};	

	// Leadscrew list
	states[24] = {25, 	25, 	0,		1,		2, LIST, "1-start"};	
	states[25] = {24, 	24, 	0,		1,		2, LIST, "4-start"};	

	// Enable-motor list
	states[29] = {32, 	30, 	0,		1,		2, LIST, "A"};	
	states[30] = {29, 	31, 	0,		1,		2, LIST, "B"};	
	states[31] = {30, 	32, 	0,		1,		2, LIST, "C"};	
	states[32] = {31, 	29, 	0,		1,		2, LIST, "D"};	

// ---------------------------------------------------------------------------
//  Finish setup() initialisation
// ---------------------------------------------------------------------------

	// Initialise stepper drivers (all start stopped)
  stepper_a.begin(motors[0].step_pin, motors[0].dir_pin); stepper_a.spin(0);
	stepper_b.begin(motors[1].step_pin, motors[1].dir_pin);	stepper_b.spin(0);
	stepper_c.begin(motors[2].step_pin, motors[2].dir_pin);	stepper_c.spin(0);
	stepper_d.begin(motors[3].step_pin, motors[3].dir_pin);	stepper_d.spin(0);

	// Task 1 for handle mqtt reception
	xTaskCreatePinnedToCore(
		Task1code,   /* Task function. */
		"Task1",     /* name of task. */
		10000,       /* Stack size of task */
		NULL,        /* parameter of the task */
		1,           /* priority of the task */
		&Task1,      /* Task handle to keep track of created task */
		0);          /* pin task to core 0 */
	delay(500);

	// Task 2 for handle motor movemets on core 0
  xTaskCreatePinnedToCore(
    TaskRunMotors, 	/* Task function. */
		"TaskMotors", 	/* name of task. */
		4096, 					/* Stack size of task */
		NULL, 					/* parameter of the task */
		1, 							/* priority of the task */
		&TaskMotors, 		/* Task handle to keep track of created task */
		1);							/* pin task to core 0 */
	delay(500);

	// Networking (optional)
	if(INTERNET_CONTROL)
	{
		setup_wifi();
		client.setServer(mqtt_server, mqtt_port);
		client.setCallback(callback);
	}
	lcd.clear();
	printScreen();
}

// ──────────────────────────────────────────────────────────────
//  SECTION 6 – MAIN LOOP & RTOS TASKS
// ──────────────────────────────────────────────────────────────

// ----------------- loop() -----------------
void loop()
{
	if( key_change)
	{
		key_change = false;
		key = getKey();
		processKey(key);
	}
}

// --------------- Task1code ----------------
// Core-0 task: Wi-Fi / MQTT handling
void Task1code( void * pvParameters )
{
 	Serial.print("Task1 running on core ");
 	Serial.println(xPortGetCoreID());

	for(;;)
	{
		if(INTERNET_CONTROL)
		{
			if (bussy_mqtt == false)
			{
				if (!client.connected())
				{
			    	reconnect();
				}
			}
			client.loop();
 		}
		vTaskDelay(10);   // yield ~10 ms
	}
}

// ------------- TaskRunMotors --------------
// Core-1 task: ContinuousStepper loops
void TaskRunMotors( void * pvParameters )
{
 	Serial.print("Task2 running on core ");
 	Serial.println(xPortGetCoreID());

	for(;;)
	{
		stepper_a.loop(); // must be called frequently
		stepper_b.loop(); 
		stepper_c.loop(); 
		stepper_d.loop(); 
    vTaskDelay(10);  // yield
  }
}

// ---------------- getKey() ----------------
// Non-blocking keypad reader
char getKey()
{
	char keys[] = "123A456B789C*0#DNF";  //  N = NoKey, F = Fail
	uint8_t index = keypad.getKey();
	
	if(keys[index] != 'N')
	{
		Serial.print("Tecla presionada: ");
		Serial.println(keys[index]);
	}
	return keys[index];
}

// ──────────────────────────────────────────────────────────────
//  SECTION 7 – UI HANDLERS (processKey, printScreen)
// ──────────────────────────────────────────────────────────────

void processKey(char _key)
{
	if(_key != 'N')
	{
		// -------- * (STAR) toggles RUN / STOP -------
		if(_key == '*')
		{
			motors[actual_motor].state = !motors[actual_motor].state;
			if(motors[actual_motor].state)
			{
				Serial.print("RUN ");
				ioport.digitalWrite(motors[actual_motor].en_pin, LOW);

				// MODIFICATION: ensure DIR pin matches direction before spinning
        digitalWrite(motors[actual_motor].dir_pin, motors[actual_motor].direction);

		    if( actual_motor == MOTOR_A){stepper_a.spin(motors[actual_motor].rpm);}
		    if( actual_motor == MOTOR_B){stepper_b.spin(motors[actual_motor].rpm);}
		    if( actual_motor == MOTOR_C){stepper_c.spin(motors[actual_motor].rpm);}
		    if( actual_motor == MOTOR_D){stepper_d.spin(motors[actual_motor].rpm);}
			}
			else
			{
				Serial.print("STOP");
				ioport.digitalWrite(motors[actual_motor].en_pin, HIGH);
				if( actual_motor == MOTOR_A){stepper_a.spin(0); stepper_a.stop();}
			  if( actual_motor == MOTOR_B){stepper_b.spin(0); stepper_b.stop();}
			  if( actual_motor == MOTOR_C){stepper_c.spin(0); stepper_c.stop();}
			  if( actual_motor == MOTOR_D){stepper_d.spin(0); stepper_d.stop();}
			}
		}

		// ---- CHECK NEXT-STATE VIA MENU KEYS --------
		if(_key == 'A')      // Flow
		{
			next_state = states[current_state].button[3];
		}
		else if(_key == 'B') // Dir
		{
			next_state = states[current_state].button[4];
		}
		else if(_key == 'C') // Cycle motor A→B→C→D
		{
			actual_motor += 1;
			if(actual_motor >= 4){actual_motor = 0;}
		}
		else if(_key == '2' and (current_state!= 1 and current_state != 11)) // Up
		{
			next_state = states[current_state].button[0];
		}
		else if(_key == '8' and (current_state!= 1 and current_state != 11) ) // Down
		{
			next_state = states[current_state].button[1];
		}
		else if(_key == '#') // Enter
		{
			next_state = states[current_state].button[2];
		}
		else
		{
			// ---------- Numeric / dot input handling ------------
			if (current_state == 1 or current_state == 11)
			{
				if (_key == '*') {      // clear buffer
            num_input = "";
        }
				else if( _key == 'D'){  // decimal point
					num_input +='.';
				}
				else {
					num_input +=_key;     // digit
				}
				lcd.setCursor(1, 1);
				lcd.print("                ");
        lcd.setCursor(1, 1);
				lcd.print(num_input);
			}
		}

		Serial.print("Next state : ");
		Serial.println(next_state);
		Serial.print("Current state : ");
		Serial.println(current_state);

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
				//setMicrostep(actual_motor, motors[actual_motor].microstep_type);

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
				
				// MODIFICATION: set DIR pin based on direction ( Infuse vs Withdraw )
        digitalWrite(motors[actual_motor].dir_pin, motors[actual_motor].direction);
			}
			else if(current_state == 1) // target flow input
			{
				motors[actual_motor].flow = num_input.toFloat();
				num_input = "";
				calculateNewStep();
				saveMotorPreset(actual_motor);
				Serial.print("new flow rate set: ");
				Serial.println(motors[actual_motor].flow);
				lcd.setCursor(14,1);
				
				Serial.print("Unit type: ");
				if (motors[actual_motor].unit_type == UNIT_UL_MIN) {
					Serial.println("uL/min");
				} else if (motors[actual_motor].unit_type == UNIT_UL_HR) {
					Serial.println("uL/h");
				} else if (motors[actual_motor].unit_type == UNIT_ML_MIN) {
					Serial.println("mL/min");
				} else if (motors[actual_motor].unit_type == UNIT_ML_HR) {
					Serial.println("mL/h");
				} else {
					Serial.println("Unknown unit");
				}

				lcd.print("Ok");
				delay(1000);
			}
			else if(current_state == 11) // diameter input
			{
				motors[actual_motor].diameter = num_input.toFloat();
				num_input = "";
				calculateNewStep();	
				saveMotorPreset(actual_motor);				
				Serial.print("new diameter set: ");		
				Serial.println(motors[actual_motor].diameter);
				lcd.setCursor(14,1);
				lcd.print("Ok");
				delay(1000);
			}
		}
		if(next_state == 1)
		{
			Serial.print("next_state: ");
			Serial.print(next_state);
			Serial.print("; current_state :");
			Serial.println(current_state);
			if (current_state != 1)
			{
				lcd.setCursor(1,1);
				lcd.print("              ");
			}
		}
		if(next_state == 11)
		{
			Serial.println ("New state diameter");
			Serial.print("next_state: ");
			Serial.print(next_state);
			Serial.print("; current_state :");
			Serial.println(current_state);
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
	Serial.println("-------------------------");/home/wac/Documents/repo/iowlabs/EEG/firmware/sketch_aug7a_copy_20240821150535/sketch_aug7a_copy_20240821150535.ino

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
		lcd.setCursor(0,1); lcd.print("                ");
		lcd.setCursor(1,1);
		if(motors[actual_motor].state){lcd.print("RUN"); }
		else{lcd.print("STOP");}
	}

	// FLOW-RATE entry screen
	if(current_state == 1)
	{
		lcd.setCursor(0,0);	 lcd.print("                ");
		lcd.setCursor(0,0);	 lcd.print("FLOW: ");
		lcd.setCursor(15,0); lcd.print(motors[actual_motor].name);
		
		if(motors[actual_motor].unit_type == UNIT_UL_MIN)lcd.print("[uL/min]");
		if(motors[actual_motor].unit_type == UNIT_UL_HR)lcd.print("[uL/hr]");
		if(motors[actual_motor].unit_type == UNIT_ML_MIN)lcd.print("[mL/min]");
		if(motors[actual_motor].unit_type == UNIT_ML_HR)lcd.print("[mL/hr]");
		
		lcd.setCursor(0,1);  lcd.print(">");  // input cursor
	}

	// DIAMETER entry
	if(current_state == 11)
	{
		lcd.setCursor(0,0);	 lcd.print("                ");
		lcd.setCursor(0,0);	 lcd.print("Diameter(mm):");
		lcd.setCursor(15,0); lcd.print(motors[actual_motor].name);
		lcd.setCursor(0,1);  lcd.print(">");
	}

	// LIST & LIST_FILL redraws
	if (states[current_state].type == LIST)
	{
		lcd.clear();

		Serial.print(F("> "));
    Serial.println(states[current_state].text);
		lcd.setCursor(15,0);
		lcd.print(motors[actual_motor].name);
		lcd.setCursor(0,0);
		lcd.print("> ");
		lcd.print(states[current_state].text);

		if(current_state > 11 and current_state < 25)
		{
			if( current_state == 13 and motors[actual_motor].unit_type == UNIT_UL_MIN) lcd.print(" *");
			if( current_state == 14 and motors[actual_motor].unit_type == UNIT_UL_HR) lcd.print(" *");
			if( current_state == 15 and motors[actual_motor].unit_type == UNIT_ML_MIN) lcd.print(" *");
			if( current_state == 16 and motors[actual_motor].unit_type == UNIT_ML_HR) lcd.print(" *");

			Serial.print("current_state on motor ");
			Serial.print(motors[actual_motor].name);
			Serial.print(" : ");
			Serial.print(current_state);
			Serial.print(" unit_type: ");
			Serial.println(motors[actual_motor].unit_type);

			if( current_state == 17 and motors[actual_motor].gearbox_type == GEAR_BOX_1_1) lcd.print(" *");
			if( current_state == 18 and motors[actual_motor].gearbox_type == GEAR_BOX_1_25) lcd.print(" *");
			if( current_state == 19 and motors[actual_motor].gearbox_type == GEAR_BOX_1_100) lcd.print(" *");

			if( current_state == 20 and motors[actual_motor].microstep_type == MICRO_STEP_1_8) lcd.print(" *");
			if( current_state == 21 and motors[actual_motor].microstep_type == MICRO_STEP_1_16) lcd.print(" *");
			if( current_state == 22 and motors[actual_motor].microstep_type == MICRO_STEP_1_32) lcd.print(" *");
			if( current_state == 23 and motors[actual_motor].microstep_type == MICRO_STEP_1_64) lcd.print(" *");
			setMicrostep(actual_motor, motors[actual_motor].microstep_type);

			if( current_state == 24 and motors[actual_motor].rod_type == ROD_1_STAR) lcd.print(" *");
			if( current_state == 25 and motors[actual_motor].rod_type == ROD_4_STAR) lcd.print(" *");
		}

		// show next-item on second row
    	if (states[current_state].button[1] < 200)
    	{
      	Serial.print(F("  "));
      	Serial.println(states[states[current_state].button[1]].text);
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
				setMicrostep(actual_motor, motors[actual_motor].microstep_type);

				if( current_state == 25 and motors[actual_motor].rod_type == ROD_1_STAR) lcd.print(" *");
				if( current_state == 24 and motors[actual_motor].rod_type == ROD_4_STAR) lcd.print(" *");
			}
		}
	}
	
	if (states[current_state].type == LIST_FILL)
	{
		lcd.clear();

		Serial.print(F("> "));
    Serial.println(states[current_state].text);
		lcd.setCursor(15,0);
		lcd.print(motors[actual_motor].name);
		lcd.setCursor(0,0);
		lcd.print("> ");
		lcd.print(states[current_state].text);

		if(current_state == 2 and motors[actual_motor].direction == RIGHT_DIR ) lcd.print(" *");
		if(current_state == 3 and motors[actual_motor].direction == LEFT_DIR ) lcd.print(" *");

		Serial.print(F("  "));
		Serial.println(states[states[current_state].button[4]].text);
		lcd.setCursor(0,1);

		lcd.print("  ");
		lcd.print(states[states[current_state].button[4]].text);

		if(current_state == 3 and motors[actual_motor].direction == RIGHT_DIR ) lcd.print(" *");
		if(current_state == 2 and motors[actual_motor].direction == LEFT_DIR ) lcd.print(" *");
	}
}

// ──────────────────────────────────────────────────────────────
//  SECTION 8 – MOTION CALCS & LEGACY MOVE HELPERS
// ──────────────────────────────────────────────────────────────

// -----------------------------------------------------------------------------
//  setMicrostep() – drive MS1 / MS2 lines via PCA9555
// -----------------------------------------------------------------------------
void setMicrostep(uint8_t motor, uint16_t microstep)
{
  bool ms1 = false, ms2 = false;

	switch (microstep) {
		case 8:  	ms1 = false; ms2 = false; break;   // 1/8 microstepping
		case 16:  ms1 = true;  ms2 = false; break;  // 1/16 microstepping
		case 32:  ms1 = false; ms2 = true;  break;  // 1/32 microstepping
		case 64:  ms1 = true;  ms2 = true;  break;  // 1/64 microstepping
	}

  ioport.digitalWrite(MS1_PIN[motor], ms1);
  ioport.digitalWrite(MS2_PIN[motor], ms2);
}


// -------- calculateNewStep() (new detailed version) ----------
void calculateNewStep()
{
    // 1) Grab the raw parameters for the currently selected motor
    float _flow       	 = motors[actual_motor].flow;          // e.g., “3000” if typed as 3000 μL/h
    float _diameter		   = motors[actual_motor].diameter;      // e.g., 8.17 mm
    int   _gearbox       = motors[actual_motor].gearbox_type;  // e.g., 25 for a 1:25 gearbox
    int   _rod			     = motors[actual_motor].rod_type;      // e.g., 2 for a 2 mm leadscrew
    int   _microstep     = motors[actual_motor].microstep_type; // e.g., 16 for 1/16 microstep
    float _unit			     = motors[actual_motor].unit_type;     // e.g., 1/60 if original flow was in μL/h

    // 2) Convert “raw flow” into μL per minute:
    //    If user typed “3000” and unit_type = (1/60), then:
    //      desired_flow_ul_per_min = 3000 × (1/60)  = 50 μL/min
    float desired_flow_ul_per_min = _flow * _unit;

    // 3) Compute syringe cross‐sectional area (in mm²):
    //    A = π · (D/2)²
    float radius_mm = _diameter / 2.0f;
    float cross_section_mm2 = PI * radius_mm * radius_mm;   // mm²

    // 4) Compute how many μL come out per full revolution of the leadscrew:
    //    vol_per_rev_ul = cross_section_mm2 (mm²) × rod_lead_mm (mm/rev)
    //                   = mm³/rev = μL/rev
    float vol_per_rev_ul = cross_section_mm2 * _rod;

    // 5) Now compute the needed RPM (revolutions per minute):
    //    RPM = (desired_flow_ul_per_min) / (vol_per_rev_ul)
    float motor_rpm = desired_flow_ul_per_min / vol_per_rev_ul;

		motor_rpm *= _gearbox;
    motor_rpm *= _microstep;

    // 6) Compute how many steps correspond to one full revolution of the motor:
    //    steps_per_rev = (motor native steps/rev) × (microstep) × (gearbox_ratio)
    //    e.g. 200 × 16 × 1  = 3200 steps/rev if gearbox = 1:1 & 1/16 microstep
    float motor_steps_per_rev = STEP_PER_REV * _microstep * _gearbox;

    // 7) Convert RPM → steps per minute:
    //    steps_per_min = motor_rpm × steps_per_rev
    float steps_per_min = motor_rpm * motor_steps_per_rev;

    // 8) If you need “steps per second” (for many stepper libraries), divide by 60:
    float steps_per_sec = steps_per_min / 60.0f;

    // 9) Finally, store these back into your motor struct:
    //    If your code expects “motors[].rpm” to hold the RPM value:
    motors[actual_motor].rpm = motor_rpm;

    // If your code expects “motors[].steps” to hold “steps per second,” then:
    motors[actual_motor].steps = int(round(steps_per_sec));

		Serial.print("Steps per sec: ");
		Serial.println(motors[actual_motor].steps);

		Serial.print("Expected microstep setting: ");
		Serial.println(motors[actual_motor].microstep_type);

    // 10) Persist the settings to EEPROM (if desired) and print debug info:
    saveMotorPreset(actual_motor);

    Serial.println(F("------------------"));
    
		Serial.print(F("Motor         : "));  Serial.println(motors[actual_motor].name);
    
		Serial.print(F("Desired flow  : "));  Serial.print(desired_flow_ul_per_min);
    Serial.println(F(" μL/min"));

    Serial.print(F("Syringe Dia  : "));   Serial.print(_diameter);
    Serial.println(F(" mm"));

    Serial.print(F("Lead_screw P  : "));  Serial.print(_rod);
    Serial.println(F(" mm/rev"));

    Serial.print(F("Area (mm²)    : "));  Serial.println(cross_section_mm2, 4);

    Serial.print(F("Vol/rev      : "));   Serial.print(vol_per_rev_ul, 4);
    Serial.println(F(" μL/rev"));

    Serial.print(F(" → RPM set    : "));  Serial.print(motor_rpm, 4);
    Serial.println(F(" rev/min"));

    Serial.print(F("Steps/rev    : "));   Serial.println(motor_steps_per_rev, 0);

    Serial.print(F("Steps/sec    : "));  Serial.println(steps_per_sec, 2);

    Serial.println(F("------------------"));
}

// ------------- Legacy manual-pulse helpers -------------------
void moveStepper(int _steps, int _ch)
{
	ioport.digitalWrite(motors[_ch].en_pin, LOW);
	digitalWrite(motors[_ch].dir_pin, motors[_ch].direction); // Configurar dirección (puedes ajustar según necesites)

	for (int j = 0; j < _steps; j++)
	{
    	digitalWrite(motors[_ch].step_pin, HIGH);
    	delayMicroseconds(500); // Ajusta según la velocidad deseada
    	digitalWrite(motors[_ch].step_pin, LOW);
    	delayMicroseconds(500); // Ajusta según la velocidad deseada
  	}
  	ioport.digitalWrite(motors[_ch].en_pin, HIGH); // Deshabilitar el motor
}

/* LEGACY FUNTION (not used in this code): Moves all enabled motors*/
void moveMotors()
{
	for (size_t i = 0; i < 4; i++)
	{
		if(motors[i].state)ioport.digitalWrite(motors[i].en_pin, LOW);
		digitalWrite(motors[i].dir_pin, motors[i].direction); // Configurar dirección (puedes ajustar según necesites)
	}
	for (int j = 0; j < STEPS_DEFAULT; j++)
	{
    	for (size_t i = 0; i < 4; i++) {digitalWrite(motors[i].step_pin, HIGH);}
    	delayMicroseconds(500); // Ajusta según la velocidad deseada
    	for (size_t i = 0; i < 4; i++) {digitalWrite(motors[i].step_pin, LOW);}
    	delayMicroseconds(500); // Ajusta según la velocidad deseada
  	}
  	for (size_t i = 0; i < 4; i++) {ioport.digitalWrite(motors[i].en_pin, HIGH);}// Deshabilitar el motor
}

// ---------------- ISR – keypad interrupt ---------------------
void keyChanged()
{
	key_change = true;
}

// ──────────────────────────────────────────────────────────────
//  SECTION 9 – PRESET STORAGE & NETWORKING UTILITIES
// ──────────────────────────────────────────────────────────────

// ---------------- saveMotorPreset -----------------
void saveMotorPreset(int index)
{
	String ns = "motor_" + String(MOTORS_NAMES[index]);
  preferences.begin(ns.c_str(), false);						//  write mode
  	
  preferences.putChar ("name",   motors[index].name);
  preferences.putInt  ("steps",  motors[index].steps);
  preferences.putInt  ("rpm",    motors[index].rpm);
  preferences.putFloat("flow",   motors[index].flow);
  preferences.putFloat("diam",   motors[index].diameter);
  preferences.putInt  ("gear",   motors[index].gearbox_type);
  preferences.putInt  ("rod",    motors[index].rod_type);
  preferences.putInt  ("micro",  motors[index].microstep_type);
  preferences.putFloat("unit",   motors[index].unit_type);
  preferences.putBool ("dir",    motors[index].direction);
  preferences.putBool ("state",  motors[index].state);

  preferences.end();
}

// ---------------- loadMotorPresets ----------------
void loadMotorPresets(int index)
{
  String ns = "motor_" + String(MOTORS_NAMES[index]);
  preferences.begin(ns.c_str(), true);           // read mode

  motors[index].name            = preferences.getChar ("name",   MOTORS_NAMES[index]);
  motors[index].step_pin        = preferences.getInt  ("step_pin",STEP_PINS[index]);
  motors[index].dir_pin         = preferences.getInt  ("dir_pin", DIR_PINS[index]);
  motors[index].en_pin          = preferences.getInt  ("en_pin",  EN_PINS[index]);
  motors[index].steps           = preferences.getInt  ("steps",   STEPS_DEFAULT);
  motors[index].rpm             = preferences.getInt  ("rpm",     RPM_DEFAULT);
  motors[index].flow            = preferences.getFloat("flow",    FLOW_DEFAULT);
  motors[index].diameter        = preferences.getFloat("diam",    DIAMETER_DEFAULT);
  motors[index].gearbox_type    = preferences.getInt  ("gear",    GEAR_BOX_DEFAULT);
  motors[index].rod_type        = preferences.getInt  ("rod",     ROD_DEFAULT);
  motors[index].microstep_type  = preferences.getInt  ("micro",   MICROSTEP_DEFAULT);
  motors[index].unit_type       = preferences.getFloat("unit",    UNIT_TYPE_DEFAULT);
  motors[index].direction       = preferences.getBool ("dir",     DIR_DEFAULT);
  motors[index].state           = preferences.getBool ("state",   STATE_DEFAULT);

  preferences.end();
}

// ──────────────────────────────────────────────────────────────
//  SECTION 10 – MQTT CALLBACK & PUBLISH HELPERS
// ──────────────────────────────────────────────────────────────

// -------------- callback (MQTT RX) ---------------
void callback(char* topic, byte* payload, unsigned int length)
{
	Serial.print("Mensaje recibido en el tema: ");
	Serial.println(topic);
	Serial.print("Contenido: ");
	for (unsigned int i = 0; i < length; i++)
 	{
		Serial.print((char)payload[i]);
	}
	Serial.println("");
	processCmd(payload,length);
}

// ---------------- publishMqtt --------------------
void publishMqtt(char *payload)
{
	if(wifi_status)
	{
		bussy_mqtt = true;
		Serial.print("sending data by mqtt . . .");
		client.publish(PUBLISH_TOPIC, payload , true);
		Serial.println(" done");
		bussy_mqtt = false;
	}

}

// ──────────────────────────────────────────────────────────────
//  SECTION 11 – WIFI / MQTT CONNECTION ROUTINES
// ──────────────────────────────────────────────────────────────

void setup_wifi()
{
	delay(10);
	Serial.println("Conectando a Wi-Fi...");
	WiFi.begin(ssid, password);
	wifi_try = 0;
 	while (WiFi.status() != WL_CONNECTED)
	{
    	delay(1000);
    	Serial.print(".");
		wifi_try +=1;
		if(wifi_try >= WIFI_TIMEOUT)break;
  	}
	if(WiFi.status()!= WL_CONNECTED)
	{
		wifi_status = false;
		Serial.println(" Wifi no conectado");
	}
	else
	{
		wifi_status = true;
		Serial.println("\nConexión Wi-Fi establecida. Dirección IP: ");
	  	Serial.println(WiFi.localIP());
	}


}

// ---------------- reconnect (MQTT) ---------------
void reconnect()
{
	if(wifi_status)
	{
		mqtt_try = 0 ;
		while (!client.connected())
		{
	    	Serial.print("Conectando al broker MQTT...");
	    	if (client.connect(ID, mqtt_user, mqtt_password))
			{
	      		Serial.println("Conectado");
	      		// Suscribirse al tema con QoS 1
	      		client.subscribe(SUBSCRIBE_TOPIC, 1);
	      		Serial.println("Suscrito al tema: ");
	      		Serial.println(SUBSCRIBE_TOPIC);
	    }
			else
			{
	      		Serial.print("Error: ");
	      		Serial.print(client.state());
	      		Serial.println(". Reintentando en 5 segundos...");
	      		delay(5000);
				mqtt_try +=1;
				if(mqtt_try >= MQTT_TIMEOUT)break;
	    }
	  }
	}

}

// -- publishResponse: simple OK/ERROR wrapper -----
void publishResponse()
{
  StaticJsonDocument<256> doc_tx;
	doc_tx["id"] 	= ID;
	doc_tx["cmd"]  	= cmd;
	doc_tx["resp"]  = response;

	String json;
	serializeJson(doc_tx, json);
	Serial.println(json);

	publishMqtt((char*) json.c_str());

}

// ──────────────────────────────────────────────────────────────
//  SECTION 12 – processCmd (JSON command parser)
// ──────────────────────────────────────────────────────────────

void processCmd(byte* payload, unsigned int length)
{
	StaticJsonDocument<1024> doc_rx;
    //const char* json_rx = "{\"id\":\"anw00\",\"cmd\":\"gps\",\"arg\":1}";
    DeserializationError error_rx;
    //check for error
    error_rx = deserializeJson(doc_rx, payload,length);
    if (error_rx)
    {
		Serial.print(F("deserializeJson() failed: "));
		response = RESPONSE_ERROR_JSON;
		publishResponse();
      	Serial.println(error_rx.c_str());
    }
		Serial.println("prosesing Data");

    //parsing incoming msg
    rcvd_id = doc_rx["ID"];

    if( strcmp(rcvd_id,ID) == 0 )
    {
		JsonObject arg_js = doc_rx["arg"];

      	cmd = doc_rx["cmd"];

      	//prossesing incoming command
      	if(strcmp(cmd,"rst")==0)
      	{
        	response = RESPONSE_OK;
        	publishResponse();
			ESP.restart();
      	}
      	else if(strcmp(cmd,"v")==0)
      	{
			response = VERSION;
        	publishResponse();

      	}
		else if(strcmp(cmd,"diameter")==0)// set diameter to motor
      	{
			float _diameter  = arg_js["diameter"];
			uint8_t _motor = arg_js["motor"];

			motors[_motor].diameter = _diameter;
			response = RESPONSE_OK;
        	publishResponse();
      	}
		else if(strcmp(cmd,"flow")==0)// set diameter to motor
      	{

			float _flow  = arg_js["flow"];
			uint8_t _motor = arg_js["motor"];

			motors[_motor].flow = _flow;
			response = RESPONSE_OK;
        	publishResponse();
      	}
		else if(strcmp(cmd,"gearbox")==0)// set diameter to motor
      	{

			int _gearbox  = arg_js["gearbox"];
			uint8_t _motor = arg_js["motor"];

			motors[_motor].gearbox_type = _gearbox;
			response = RESPONSE_OK;
        	publishResponse();
      	}
		else if(strcmp(cmd,"gearbox")==0)// set diameter to motor
      	{
			int _gearbox  = arg_js["gearbox"];
			uint8_t _motor = arg_js["motor"];

			motors[_motor].gearbox_type = _gearbox;
			response = RESPONSE_OK;
        	publishResponse();
      	}
		else if(strcmp(cmd,"gearbox")==0)// set diameter to motor
      	{
			int _gearbox  = arg_js["gearbox"];
			uint8_t _motor = arg_js["motor"];

			motors[_motor].gearbox_type = _gearbox;
			response = RESPONSE_OK;
        	publishResponse();
      	}
		else if(strcmp(cmd,"rod")==0)// set diameter to motor
      	{
			float _rod  = arg_js["rod"];
			uint8_t _motor = arg_js["motor"];

			motors[_motor].rod_type = _rod;
			response = RESPONSE_OK;
        	publishResponse();
      	}
		else if(strcmp(cmd,"rpm")==0)// set diameter to motor
		{
			float _rpm  = arg_js["rpm"];
			uint8_t _motor = arg_js["motor"];

			motors[_motor].rpm = _rpm;
			response = RESPONSE_OK;
			publishResponse();
		}
		else if(strcmp(cmd,"microstep")==0)// set diameter to motor
		{

			int _ustep  = arg_js["microstep"];
			uint8_t _motor = arg_js["motor"];

			motors[_motor].microstep_type = _ustep;
			response = RESPONSE_OK;
			publishResponse();
		}
		else if(strcmp(cmd,"unit")==0)// set diameter to motor
		{
			float _unit  = arg_js["unit"];
			uint8_t _motor = arg_js["motor"];

			motors[_motor].unit_type = _unit;
			response = RESPONSE_OK;
			publishResponse();
		}
		else if(strcmp(cmd,"dir")==0)// set diameter to motor
		{

			bool _dir  = arg_js["dir"];
			uint8_t _motor = arg_js["motor"];

			motors[_motor].direction = _dir;
			response = RESPONSE_OK;
			publishResponse();
		}
		else if(strcmp(cmd,"state")==0)// set diameter to motor
		{

			bool _en  = arg_js["state"];
			uint8_t _motor = arg_js["motor"];

			motors[_motor].state = _en;
			response = RESPONSE_OK;
			publishResponse();
		}

      	else
      	{
        	Serial.println("Command not valid");
			response = RESPONSE_ERROR_CMD;
			publishResponse();
      	}
      	cmd = "";

    }
    else
    {
      Serial.println("msg not for me");
    }
}

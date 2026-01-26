/*

Author: WAC@LibreHub
Versión of code: V3.4

stepper conextions

Rojo   A+
Verde  A-
Negro  B+
Azul   B-

This version of the code performance:

- implements the new flowchart
- implements GUI updated
- Controls steppers motor by controlling the rpm.
- save setup to flash with preference.h
- add comments


Libraries:

- 	xreef/PCF8574 library @ ^2.3.7  #GPIO extensor
	robtillaart/I2CKeyPad@^0.5.0  	#Keypad
    marcoschwartz/LiquidCrystal_I2C @ ^1.1.4 #LCD I2C display
	bblanchon/ArduinoJson@^6.19.4			# Json
	knolleary/PubSubClient @ ^2.8			# mqtt client
	bblanchon/ContinuousStepper @ ^3.1.0	# control stepper motors by rpm

*/


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

#define KEYPAD_ADDR 0x24  	// Dirección I2C del PCF8574A
#define GPIO_ADDR 0x20 		// Dirección I2C del PCF8574A
#define LCD_ADDR 0x27      	// Dirección I2C de la pantalla LCD

#define RUN 1
#define STOP 0

#define RIGHT_DIR 1
#define LEFT_DIR 0

#define MOTOR_A 0
#define MOTOR_B 1
#define MOTOR_C 2
#define MOTOR_D 3

#define GEAR_BOX_1_1 	1
#define GEAR_BOX_1_25 	25
#define GEAR_BOX_1_100 	100

#define ROD_1_STAR 	2.5 //mm
#define ROD_4_STAR 	8   //mm

#define MICRO_STEP_1_8 		200
#define MICRO_STEP_1_16 	400
#define MICRO_STEP_1_32 	800
#define MICRO_STEP_1_64 	1600

#define UNIT_UL_HR 	1000.0
#define UNIT_ML_HR 	1.0
#define UNIT_UL_MIN	16.67
#define UNIT_ML_MIN 0.01667

#define LEAD 2.5 // mm/rev
#define STEP_PER_REV 200 //(200 step/revolution)

#define IDLE 0
#define SETTINGS 1

#define N_STATES 34
#define NUM_MOTORS 4

#define STEPS_DEFAULT 		1000
#define RPM_DEFAULT 		100.0
#define FLOW_DEFAULT 		100.0
#define DIAMETER_DEFAULT 	48.5
#define GEAR_BOX_DEFAULT  	GEAR_BOX_1_1
#define MICROSTEP_DEFAULT   MICRO_STEP_1_8
#define UNIT_TYPE_DEFAULT   UNIT_UL_HR
#define ROD_DEFAULT 		ROD_1_STAR
#define DIR_DEFAULT			RIGHT_DIR
#define ENABLED_DEFAULT		false

#define VERSION  "SPC - V3.4"

enum listOfItemTypes {ACTIVITY,TEXT, LIST, DATA, LIST_MOTOR,LIST_FILL};

// Pines del motor stepper
int EN_PINS[] 	= {15,11,7,3};
int STEP_PINS[] = {4,17,26,33};
int DIR_PINS[]  = {16,18,25,32};
char MOTORS_NAMES[] = {'A','B','C','D'};

/*
	INSTANCES
*/

//Expansor GPIO for drivers
PCA9555 ioport(GPIO_ADDR);

//Expansor GPIO for keypad
I2CKeyPad keypad(KEYPAD_ADDR);

//LCD display
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

//Task to handel motor loop
TaskHandle_t TaskMotors;

//Task to handel mqtt rcv commands
TaskHandle_t Task1;

//Handel save pre-sets for new instance
Preferences preferences;

// Objetos para cada motor
ContinuousStepper<StepperDriver> stepper_a;
ContinuousStepper<StepperDriver> stepper_b;
ContinuousStepper<StepperDriver> stepper_c;
ContinuousStepper<StepperDriver> stepper_d;


/*
	VARIABLES
*/
uint32_t start, stop;
uint32_t lastKeyPressed = 0;

bool key_change = false;

const char keyMap[16] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};

struct MenuStates
{
	uint8_t button[6];
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
	int rod_type;
	int microstep_type;
	float unit_type;
	bool direction;
	bool enabled;
};
Motor motors[4];

uint8_t next_state = 1;
uint8_t current_state = 0;
MenuStates states[N_STATES];

uint8_t	estado_pantalla = IDLE;
uint8_t estado_motor = STOP;
char actual_motor = MOTOR_A;
char key;
String num_input = "";

char getKey();
void calculateNewStep();
void moveStepper(int _steps, int _ch);
void moveMotors();
void processKey(char _key);
void keyChanged();
void printScreen();
void saveMotorPreset(int index);
void loadMotorPresets(int index);
void Task1code(void *pvParameters);
void TaskRunMotors( void * pvParameters );

void setup()
{
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

	//  NOTE: PCF8574 will generate an interrupt on key press and release.
	pinMode(5, INPUT_PULLUP);
	attachInterrupt(5, keyChanged, FALLING);// only release
	key_change = false;

	Serial.begin(9600);

	ioport.begin();

	//Define pin modes
	for(int i = 0;i<4;i++)
	{
		ioport.pinMode(EN_PINS[i], OUTPUT);
		ioport.digitalWrite(EN_PINS[i], LOW);

		pinMode(DIR_PINS[i],OUTPUT);
		pinMode(STEP_PINS[i],OUTPUT);
	}


	//STATE MACHINE
	// 			  UP DOWN 	ENTER 	Flow 	DIR		 MOTOR
	states[0]  = {0, 	0, 		5,		1,		2,		4,  ACTIVITY, "MOTOR"};

	states[1]  = {1,   1, 		0,		1,	 	1,	 	1,  DATA, "Flow"}; 	//Flow input

	states[2]  = {2, 	2, 		0,		1,		3,		2,  LIST_FILL, "Fill"};		// fill/refill menu
	states[3]  = {3, 	3, 		0,		1,		2,		3,  LIST_FILL, "Refill"};	// fill/refill menu

	states[4]   = {4, 	4, 		0,		1,		4,		26,  LIST_MOTOR, "A"}; //MOTOR MENU
	states[26]  = {26, 	26,		0,		1,		26,		27,  LIST_MOTOR, "B"}; //MOTOR MENU
	states[27]  = {27, 	27,		0,		1,		27,		28,  LIST_MOTOR, "C"}; //MOTOR MENU
	states[28]  = {28, 	28,		0,		1,		28,		4,   LIST_MOTOR, "D"}; //MOTOR MENU

	states[5]  = {10, 	6,	   11,		1,		2,		4,  LIST, "Diametro"};	// SETTINGS MS0
	states[6]  = {5, 	7,     13,		1,		2,		4,  LIST, "Unidad"};	// SETTINGS MS1
	states[7]  = {6, 	8, 	   17,		1,		2,		4,  LIST, "Gear Box"};	// SETTINGS MS2
	states[8]  = {7, 	9, 	   20,		1,		2,		4,  LIST, "Micro step"};// SETTINGS MS3
	states[9]  = {8, 	10,	   24,		1,		2,		4,  LIST, "Varilla"}; 	// SETTINGS MS4
	states[10]  ={9, 	5, 	   29,		1,		2,		4,  LIST, "Enable"}; 	// SETTINGS MS4


	states[11] = {11, 	11, 	0,		1,		2,		4, DATA, "Diametro"}; // FLUJO
	states[12] = {12, 	12, 	0,		1,		2,		4, DATA, "Diametro OK"}; // FLUJO

	states[13]  = {16, 	14, 	0,		1,		2,		4,  LIST, "uL/hr"}; 	// Unidad
	states[14]  = {13, 	15, 	0,		1,		2,		4,  LIST, "ml/hr"}; 	// Unidad
	states[15]  = {14, 	16,   	0,		1,		2,		4,  LIST, "ul/min"}; 	// Unidad
	states[16]  = {15, 	13, 	0,		1,		2,		4,  LIST, "ml/min"}; 	// Unidad

	states[17] = {19, 	18, 	0,		1,		2,		4,  LIST, "1:1"}; 	// GearBox
	states[18] = {17, 	19, 	0,		1,		2,		4,  LIST, "1:25"};	// GearBox
	states[19] = {18, 	17, 	0,		1,		2,		4,  LIST, "1:100"};	// GearBox

	states[20] = {23, 	21, 	0,		1,		2,		4, LIST, "1/8"}; 	// MicroStep
	states[21] = {20, 	22, 	0,		1,		2,		4, LIST, "1/16"};	// MicroStep
	states[22] = {21, 	23, 	0,		1,		2,		4, LIST, "1/32"};	// MicroStep
	states[23] = {22, 	20, 	0,		1,		2,		4, LIST, "1/64"};	// MicroStep

	states[24] = {25, 	25, 	0,		1,		2,		4, LIST, "1-star"};	// Rod scew pitch
	states[25] = {24, 	24, 	0,		1,		2,		4, LIST, "4-star"};	// Rod scew pitch

	states[29] = {32, 	30, 	0,		1,		2,		4, LIST, "A"};	// Rod scew pitch
	states[30] = {29, 	31, 	0,		1,		2,		4, LIST, "B"};	// Rod scew pitch
	states[31] = {30, 	32, 	0,		1,		2,		4, LIST, "C"};	// Rod scew pitch
	states[32] = {31, 	29, 	0,		1,		2,		4, LIST, "D"};	// Rod scew pitch


	//load data from previous session
	for (size_t i = 0; i < NUM_MOTORS; i++)
	{
		loadMotorPresets(i);
	}

	// Begin stepper instances
    stepper_a.begin(motors[0].step_pin, motors[0].dir_pin);
    stepper_a.spin(0);  // Comienza detenido

	stepper_b.begin(motors[1].step_pin, motors[1].dir_pin);
	stepper_b.spin(0);  // Comienza detenido

	stepper_c.begin(motors[2].step_pin, motors[2].dir_pin);
	stepper_c.spin(0);  // Comienza detenido

	stepper_d.begin(motors[3].step_pin, motors[3].dir_pin);
	stepper_d.spin(0);  // Comienza detenido


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
		4096, 			/* Stack size of task */
		NULL, 			/* parameter of the task */
		1, 				/* priority of the task */
		&TaskMotors, 	/* Task handle to keep track of created task */
		1);				/* pin task to core 0 */
	delay(500);


	lcd.clear();
	printScreen();
}

/*
	Main loop to hanel keypad interface
*/
void loop()
{

	if( key_change)
	{
		key_change = false;
		key = getKey();
		processKey(key);
	}
}

/*
	Second loop to manage the mqtt connections
*/
void Task1code( void * pvParameters )
{
 	Serial.print("Task1 running on core ");
 	Serial.println(xPortGetCoreID());

	for(;;)
	{
		if (estado_motor == true)
		{
			Serial.println("moviendo motor" );
			delay(60000);
			//moveMotors();

			//	moveStepper(motors[3].steps, 3);/* code */

		}

    	vTaskDelay(1);  // Cede el control por un tick del sistema
  	}
}

/*
	Third loop to manage the motor moves
*/
void TaskRunMotors( void * pvParameters )
{
 	Serial.print("Task2 running on core ");
 	Serial.println(xPortGetCoreID());

	for(;;)
	{
		if(estado_motor == true)
		{
			stepper_a.loop(); // Importante: debe llamarse frecuentemente
			stepper_b.loop(); // Importante: debe llamarse frecuentemente
			stepper_c.loop(); // Importante: debe llamarse frecuentemente
			stepper_d.loop(); // Importante: debe llamarse frecuentemente
		}
    	vTaskDelay(1);  // Cede el control por un tick del sistema
  	}
}

/*
	handle keypad. return the last key pressed.
*/
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


/*
	GUI: Implementation of the logic for the state machine.
	It receives the last key pressed, calculates the next state, and updates the screens.
 */
void processKey(char _key)
{
	if(_key != 'N')
	{
		if(_key == '*')
		{
			estado_motor = ! estado_motor;
			if(estado_motor)
			{
				Serial.print("RUN ");

				for (int i = 0; i < NUM_MOTORS; i++)
				{
		      		if(motors[i].enabled)
					{
						ioport.digitalWrite(motors[i].en_pin, LOW);
		        		if( i == 0){stepper_a.spin(motors[i].rpm);}
		        		if( i == 1){stepper_b.spin(motors[i].rpm);}
		        		if( i == 2){stepper_c.spin(motors[i].rpm);}
		        		if( i == 3){stepper_d.spin(motors[i].rpm);}
					}
					else
					{
						if( i == 0){stepper_a.spin(0);}
			        	if( i == 1){stepper_b.spin(0);}
			        	if( i == 2){stepper_c.spin(0);}
			        	if( i == 3){stepper_d.spin(0);}
					}
		    	}
			}
			else
			{
				Serial.print("STOP");
				for (int i = 0; i < NUM_MOTORS; i++)
				{
					ioport.digitalWrite(motors[i].en_pin, HIGH);

		    	}
				stepper_a.stop();
				stepper_b.stop();
				stepper_c.stop();
				stepper_d.stop();
			}
		}
		else if(_key == 'A') // FLOW KEY
		{
			next_state = states[current_state].button[3];
		}
		else if(_key == 'B') // DIR KEY
		{
			next_state = states[current_state].button[4];
		}
		else if(_key == 'C') // MOTOR KEY
		{
			next_state = states[current_state].button[5];
		}
		else if(_key == '2' and (current_state!= 1 and current_state != 11)) // UP KEY
		{
			next_state = states[current_state].button[0];
		}
		else if(_key == '8' and (current_state!= 1 and current_state != 11) ) // DOWN KEY
		{
			next_state = states[current_state].button[1];
		}
		else if(_key == '#') //ENTER KEY
		{
			next_state = states[current_state].button[2];
		}
		else
		{
			if(current_state == 1 or current_state == 11)// Proccess input data
			{
				if( _key == 'D') { num_input +='.';}
				else {num_input +=_key;}
				lcd.setCursor(1,1);
				lcd.print(num_input);
			}
		}

		Serial.print("Next state : ");
		Serial.println(next_state);
		Serial.print("Current state : ");
		Serial.println(current_state);

		if( (current_state > 12 and current_state < 26)  ) //SET UNIT
		{

			if(next_state == 0)
			{
				if (current_state == 13 ) motors[actual_motor].unit_type = UNIT_UL_HR;
				if (current_state == 14 ) motors[actual_motor].unit_type = UNIT_ML_HR;
				if (current_state == 15 ) motors[actual_motor].unit_type = UNIT_UL_MIN;
				if (current_state == 16 ) motors[actual_motor].unit_type = UNIT_ML_MIN;

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

		}
		if ( current_state > 28 and current_state < 33 )
		{
			if( next_state == 0)
			{
				if (current_state == 29 ) motors[MOTOR_A].enabled = !motors[MOTOR_A].enabled;
				if (current_state == 30 ) motors[MOTOR_B].enabled = !motors[MOTOR_B].enabled;
				if (current_state == 31 ) motors[MOTOR_C].enabled = !motors[MOTOR_C].enabled;
				if (current_state == 32 ) motors[MOTOR_D].enabled = !motors[MOTOR_D].enabled;
			}
		}

		if(current_state == 4 or current_state == 26 or current_state == 27 or current_state == 28)
		{
			if(next_state == 0)
			{
				if(current_state == 4)  actual_motor = MOTOR_A;
				if(current_state == 26) actual_motor = MOTOR_B;
				if(current_state == 27) actual_motor = MOTOR_C;
				if(current_state == 28) actual_motor = MOTOR_D;
				lcd.setCursor(2,1);
				lcd.print(" Ok");
				delay(1000);
			}

		}
		if(current_state == 1) //update target flow input
		{
			if (next_state == 0)
			{
				//update flow value to motor actual_motor
				motors[actual_motor].flow = num_input.toFloat();
				//save to presets.
				saveMotorPreset(actual_motor);

				Serial.print("new flow rate setted: ");
				num_input = "";
				calculateNewStep();
				Serial.println(motors[actual_motor].flow);
				lcd.setCursor(14,1);
				lcd.print("Ok");
				delay(1000);
			}
		}
		if(current_state == 11) //update diameter input
		{
			if (next_state == 0)
			{
				motors[actual_motor].diameter = num_input.toFloat();
				//save to presets.
				saveMotorPreset(actual_motor);
				num_input = "";
				calculateNewStep();
				Serial.print("new flow rate setted: ");
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

/* Update the LCD display*/
void printScreen()
{
	Serial.println("-------------------------");
	if(current_state == 0)
	{
		lcd.setCursor(0,0);
		lcd.print("                ");
		lcd.setCursor(0,0);
		lcd.print(" MOTOR");
 		if (motors[0].enabled) {lcd.setCursor(7,0);lcd.print("A");}
 		if (motors[1].enabled) {lcd.setCursor(9,0);lcd.print("B");}
 		if (motors[2].enabled) {lcd.setCursor(11,0);lcd.print("C");}
 		if (motors[3].enabled) {lcd.setCursor(13,0);lcd.print("D");}
		lcd.setCursor(0,1);
		lcd.print("                ");
		lcd.setCursor(1,1);
		if(estado_motor){lcd.print("RUN"); }
		else{lcd.print("STOP");}
	}
	if(current_state == 1) //SET FLOW RATE
	{
		//lcd.clear();
		lcd.setCursor(0,0);
		lcd.print("                ");
		lcd.setCursor(0,0);
		lcd.print("FLOW: ");
		if(motors[actual_motor].unit_type == UNIT_ML_HR)lcd.print("[mL/hr]");
		if(motors[actual_motor].unit_type == UNIT_UL_HR)lcd.print("[uL/hr]");
		if(motors[actual_motor].unit_type == UNIT_ML_MIN)lcd.print("[mL/min]");
		if(motors[actual_motor].unit_type == UNIT_UL_MIN)lcd.print("[uL/min]");
		lcd.setCursor(0,1);
		lcd.print(">");
	}
	if(current_state == 11)
	{
		lcd.setCursor(0,0);
		lcd.print("                ");
		lcd.setCursor(0,0);
		lcd.print("Diameter:");
		lcd.setCursor(0,1);
		lcd.print(">");
	}
	if (states[current_state].type == LIST)
	{
		lcd.clear();

		Serial.print(F("> "));
    	Serial.println(states[current_state].text);

		lcd.setCursor(0,0);
		lcd.print("> ");
		lcd.print(states[current_state].text);

		if(current_state > 11 and current_state < 25)
		{
			if( current_state == 13 and motors[actual_motor].unit_type == UNIT_UL_HR) lcd.print(" *");
			if( current_state == 14 and motors[actual_motor].unit_type == UNIT_ML_HR) lcd.print(" *");
			if( current_state == 15 and motors[actual_motor].unit_type == UNIT_UL_MIN) lcd.print(" *");
			if( current_state == 16 and motors[actual_motor].unit_type == UNIT_ML_MIN) lcd.print(" *");

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

			if( current_state == 24 and motors[actual_motor].rod_type == ROD_1_STAR) lcd.print(" *");
			if( current_state == 25 and motors[actual_motor].rod_type == ROD_4_STAR) lcd.print(" *");

		}

		if(current_state > 28 and current_state < 33)
		{
			if( current_state == 29 and motors[MOTOR_A].enabled) lcd.print(" *");
			if( current_state == 30 and motors[MOTOR_B].enabled) lcd.print(" *");
			if( current_state == 31 and motors[MOTOR_C].enabled) lcd.print(" *");
			if( current_state == 32 and motors[MOTOR_D].enabled) lcd.print(" *");
		}

		//NETX STEP
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

				if( current_state == 25 and motors[actual_motor].rod_type == ROD_1_STAR) lcd.print(" *");
				if( current_state == 24 and motors[actual_motor].rod_type == ROD_4_STAR) lcd.print(" *");

			}

			if(current_state > 28 and current_state < 33)
			{
				if( current_state == 29 and motors[MOTOR_A].enabled) lcd.print(" *");
				if( current_state == 30 and motors[MOTOR_B].enabled) lcd.print(" *");
				if( current_state == 31 and motors[MOTOR_C].enabled) lcd.print(" *");
				if( current_state == 32 and motors[MOTOR_D].enabled) lcd.print(" *");
			}
		}
	}
	if (states[current_state].type == LIST_FILL)
	{
		lcd.clear();

		Serial.print(F("> "));
    	Serial.println(states[current_state].text);

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
	if (states[current_state].type == LIST_MOTOR)
	{
		lcd.clear();

		Serial.print(F("> "));
    	Serial.println(states[current_state].text);

		lcd.setCursor(0,0);
		lcd.print(" MOTOR");


		if(current_state == 4){lcd.setCursor(6,0);lcd.print(">");lcd.setCursor(7,0);lcd.print("A");}
		if(current_state == 26){lcd.setCursor(8,0);lcd.print(">");lcd.setCursor(9,0);lcd.print("B");}
		if(current_state == 27){lcd.setCursor(10,0);lcd.print(">");lcd.setCursor(11,0);lcd.print("C");}
		if(current_state == 28){lcd.setCursor(12,0);lcd.print(">");lcd.setCursor(13,0);lcd.print("D");}

	}
}

/* Calculates new rpm based on motor presets*/
void calculateNewStep()
{
	float _flow = motors[actual_motor].flow;
	float _diameter = motors[actual_motor].diameter;
	int _gearbox = motors[actual_motor].gearbox_type;
	int _rod = motors[actual_motor].rod_type;
	int _microstep = motors[actual_motor].microstep_type;
	float _unit = motors[actual_motor].unit_type;

	float motor_rpm = 4*(_flow * _gearbox)/(_rod*3.1415*_diameter*_diameter);

 	int _step = int(_flow *_diameter*_gearbox*_rod*_microstep*_unit);
	motors[actual_motor].rpm = motor_rpm;
	motors[actual_motor].steps = _step;
	saveMotorPreset(actual_motor);
	Serial.println("------------------");
	Serial.print("Motor :");
	Serial.println(motors[actual_motor].name);
	Serial.print("new rpm setted : ");
	Serial.println(motor_rpm);
	Serial.print("new step setted : ");
	Serial.println(_step);
	Serial.println("------------------");
}

/* Moves the motor _ch a number of _steps steps*/
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

/* Mueves all enabled motos*/
void moveMotors()
{
	for (size_t i = 0; i < 4; i++)
	{
		if(motors[i].enabled)ioport.digitalWrite(motors[i].en_pin, LOW);
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

/* interrupt function to key pressed */
void keyChanged()
{
	key_change = true;
}

/*
	Save current presets to memory for motor index
*/
void saveMotorPreset(int index)
{
	String ns = "motor_" + String(MOTORS_NAMES[index]);
  	preferences.begin(ns.c_str(), false);// begin in write mode
  	preferences.putChar("name", motors[index].name);
  	preferences.putInt("steps", motors[index].steps);
  	preferences.putInt("rpm", motors[index].rpm);
  	preferences.putFloat("flow", motors[index].flow);
  	preferences.putFloat("diam", motors[index].diameter);
  	preferences.putInt("gear", motors[index].gearbox_type);
  	preferences.putInt("rod", motors[index].rod_type);
  	preferences.putInt("micro", motors[index].microstep_type);
  	preferences.putFloat("unit", motors[index].unit_type);
  	preferences.putBool("dir", motors[index].direction);
  	preferences.putBool("en", motors[index].enabled);

  	preferences.end();
}

/*
	Load presets from the previous session for motor index
*/
void loadMotorPresets(int index)
{
	String ns = "motor_" + String(MOTORS_NAMES[index]);
	preferences.begin(ns.c_str(), true); // load memory in read mode
	motors[index].name 				= preferences.getChar("name", 	MOTORS_NAMES[index]);
	motors[index].step_pin 			= preferences.getInt("step_pin",STEP_PINS[index]);
	motors[index].dir_pin 			= preferences.getInt("dir_pin",	DIR_PINS[index]);
	motors[index].en_pin 			= preferences.getInt("en_pin",	EN_PINS[index]);
	motors[index].steps 			= preferences.getInt("steps", 	STEPS_DEFAULT);
	motors[index].rpm 				= preferences.getInt("rpm", 	RPM_DEFAULT);
	motors[index].flow 				= preferences.getFloat("flow", 	FLOW_DEFAULT);
	motors[index].diameter 			= preferences.getFloat("diam", 	DIAMETER_DEFAULT);
	motors[index].gearbox_type 		= preferences.getInt("gear", 	GEAR_BOX_DEFAULT);
	motors[index].rod_type 			= preferences.getInt("rod", 	ROD_DEFAULT);
	motors[index].microstep_type 	= preferences.getInt("micro", 	MICROSTEP_DEFAULT);
	motors[index].unit_type 		= preferences.getFloat("unit", 	UNIT_TYPE_DEFAULT);
	motors[index].direction 		= preferences.getBool("dir", 	DIR_DEFAULT);
	motors[index].enabled 			= preferences.getBool("en", 	ENABLED_DEFAULT);

  	preferences.end();
}

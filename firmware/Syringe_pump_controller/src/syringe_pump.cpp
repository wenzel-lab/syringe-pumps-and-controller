/*

stepper

Rojo   A+
Verde  A-
Negro  B+
Azul   B-

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

#define KEYPAD_ADDR 0x24  // Dirección I2C del PCF8574A
#define GPIO_ADDR 0x20 // Dirección I2C del PCF8574A
#define LCD_ADDR 0x27      // Dirección I2C de la pantalla LCD

#define RUN 1
#define STOP 0

#define RIGHT_DIR 1
#define LEFT_DIR 0


#define MOTOR_A 0
#define MOTOR_B 1
#define MOTOR_C 2
#define MOTOR_D 3

#define GEAR_BOX_1_1 	1
#define GEAR_BOX_1_25 	2
#define GEAR_BOX_1_100 	3

#define ROD_1_STAR 	1
#define ROD_4_STAR 	2

#define MICRO_STEP_1_8 		1
#define MICRO_STEP_1_16 	2
#define MICRO_STEP_1_32 	3
#define MICRO_STEP_1_64 	4

#define UNIT_UL_HR 	1
#define UNIT_ML_HR 	2
#define UNIT_UL_MIN	3
#define UNIT_ML_MIN 4

#define IDLE 0
#define SETTINGS 1

#define N_STATES 34

#define STEPS_DEFAULT 1000

enum listOfItemTypes {ACTIVITY,TEXT, LIST, DATA, LIST_MOTOR,LIST_FILL};

// Pines del motor stepper
int EN_PINS[] 	= {15,11,7,3};
int STEP_PINS[] = {4,17,26,33};
int DIR_PINS[]  = {16,18,25,32};

PCA9555 ioport(0x20);
I2CKeyPad keypad(KEYPAD_ADDR);
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

//Task to handel mqtt rcv commands
TaskHandle_t Task1;

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
	uint8_t step_pin;
	uint8_t dir_pin;
	uint8_t en_pin;
	int steps;
	float flow;
	float diameter;
	int gearbox_type;
	int rod_type;
	int microstep_type;
	int unit_type;
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
void Task1code(void *pvParameters);

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


				//step_pin, dir_pin, en_pin , steps, flow, diameter, gearbox,
	motors[0] = {'A',4 	,16	,15	, STEPS_DEFAULT, 1.0, 10.0, GEAR_BOX_1_1, ROD_1_STAR, MICRO_STEP_1_8 , UNIT_UL_MIN, RIGHT_DIR,true};//Estado inicial
	motors[1] = {'B',17	,18	,11	, STEPS_DEFAULT, 1.0, 10.0, GEAR_BOX_1_1, ROD_1_STAR, MICRO_STEP_1_8 , UNIT_UL_MIN, RIGHT_DIR,true};//Estado inicial
	motors[2] = {'C',26	,25	,7	, STEPS_DEFAULT, 1.0, 10.0, GEAR_BOX_1_1, ROD_1_STAR, MICRO_STEP_1_8 , UNIT_UL_MIN, RIGHT_DIR,true};//Estado inicial
	motors[3] = {'D',33	,32	,3	, STEPS_DEFAULT, 1.0, 10.0, GEAR_BOX_1_1, ROD_1_STAR, MICRO_STEP_1_8 , UNIT_UL_MIN, RIGHT_DIR,true};//Estado inicial


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

	lcd.clear();
	printScreen();
}

void loop()
{

	if( key_change)
	{
		key_change = false;
		key = getKey();
		processKey(key);
	}
}

//loop dos
void Task1code( void * pvParameters )
{
 	Serial.print("Task1 running on core ");
 	Serial.println(xPortGetCoreID());

	for(;;)
	{
		if (estado_motor == true)
		{
			Serial.println("moviendo motor" );

			moveMotors();

			//	moveStepper(motors[3].steps, 3);/* code */

		}

    	vTaskDelay(1);  // Cede el control por un tick del sistema
  	}
}

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
			}
			else
			{
				Serial.print("STOP");
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
			if(current_state == 1 or current_state == 11)
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
				lcd.setCursor(12,0);
				lcd.print(" Ok");
				delay(1000);
			}

		}

		if(current_state == 4 or current_state == 26 or current_state == 27 or current_state == 28)
		{
			if(next_state == 0)
			{
				if(current_state == 4) actual_motor = MOTOR_A;
				if(current_state == 26) actual_motor = MOTOR_B;
				if(current_state == 27) actual_motor = MOTOR_C;
				if(current_state == 28) actual_motor = MOTOR_D;
				lcd.setCursor(2,1);
				lcd.print(" Ok");
				delay(1000);
			}

		}
		if(current_state == 1)
		{
			if (next_state == 0)
			{
				motors[actual_motor].flow = num_input.toFloat();

				Serial.print("new flow rate setted: ");
				num_input = "";
				calculateNewStep();
				Serial.println(motors[actual_motor].flow);
				lcd.setCursor(14,1);
				lcd.print("Ok");
				delay(1000);
			}
		}
		if(current_state == 11)
		{
			if (next_state == 0)
			{
				motors[actual_motor].diameter = num_input.toFloat();
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



void calculateNewStep()
{
	float _flow = motors[actual_motor].flow;
	float _diameter = motors[actual_motor].diameter;
	int _gearbox = motors[actual_motor].gearbox_type;
	int _rod = motors[actual_motor].rod_type;
	int _microstep = motors[actual_motor].microstep_type;
	int _unit = motors[actual_motor].unit_type;

 	int _step = int(_flow *_diameter*_gearbox*_rod*_microstep*_unit);
	motors[actual_motor].steps = _step;
	Serial.print("new step setted : ");
	Serial.println(_step);
}

// Función para mover el motor
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

// Función para mover el motor
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

void keyChanged()
{
	key_change = true;
}


//    FILE: I2CKeypad_interrupts_1.ino
//  AUTHOR: Rob Tillaart
// PURPOSE: demo interrupt checking keyPressed
//     URL: https://github.com/RobTillaart/I2CKeyPad
//
//  interrupts are supported since version 0.2.1
//  this sketch show usage and some comparison with polling.
//
//   KEYPAD   PCF8574     UNO
//   rows     p0-p3
//   columns  p4-p7
//            IRQ         pin 2
//            SDA         A4
//            SCL         A5
//  4x4 or smaller keypad.


//  notes
//  an interrupt takes less than 10 micros() on an UNO
//  source - https://forum.arduino.cc/t/how-fast-can-i-interrupt/25884/6
//
//  At I2C 100KHz one polling takes 472 micros() on an UNO
//  this is at least 50x longer than handling a single interrupt.
//
//  Given that the interrupt is executed once per press/release and
//  polling at e.g 10Hz (to stay reactive) adds up both in CPU time used
//  and also in occupation of the I2C bus.
//
//  The PCF8574 will generate an interrupt both on press and release.
//  So this code reads the keypad on both signals!
//
//  Note: depending on keypad used some bouncing may occur
//        (saw it only during release)
//  can be solved by tracking the last interrupt in the ISR routine
//  however it is more efficient to reset the flag only after the
//  keypress is handled.
//
//  Note: multiple keypresses are not queued.


////////////////////////////////////////////////////////////////////////
//
//

#include <Arduino.h>
#include <Wire.h>
#include <I2CKeyPad.h>
#include <LiquidCrystal_I2C.h>
#include "clsPCA9555.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>




#define ID "SPC-01"
#define VERSION "SP-v0.1"
#define RESPONSE_OK     		"OK"
#define RESPONSE_ERROR  		"ERROR"
#define RESPONSE_ERROR_JSON  	"ERROR_INPUT_JSON"
#define RESPONSE_ERROR_CMD 		"ERROR_CMD"



#define N_STATES 43



//Handel implementation to eeprom
Preference memory;


struct MenuStates
{
  uint8_t button[4];
  uint8_t type;
  char *text;
  char *memory_key;
  uint32_t value;
};

enum listOfButtons {OP_A, OP_B, OP_C, OP_D,ASTERISCO,GATO, NA};
enum listOfPrograms {CHANGEVALUE = 200, SELECTFROMLIST, INFUSEVOLUME, INFUSETIME, REFILLVOLUME, REFILLFULL, CYCLEMODE, FIRMWAREINFO};
enum listOfItemTypes {TEXT, LIST, VOLUME, TIME, FLOWRATE, LENGTHPERVOLUME, DIAMETER};

uint8_t last_button;
uint8_t current_state = 1;
MenuStates states[N_STATES];

void getInput();
void printScreen();

void setup()
{
	Serial.begin(115200);


			//   A, B, C, D
	//General menu
	states[1]  = {5, 2, 0, 6, LIST, "Pump"};
	states[2]  = {1, 3, 0, 5, TEXT, "Infuse"};
  	states[3]  = {2, 4, 0, 7, TEXT, "Refill"};
  	states[4]  = {3, 5, 0, 9, TEXT, "Cycle Mode"};
	states[5]  = {4, 1, 0, 10, TEXT, "Settings"};

	//Actual pump menu
	states[6]  = {9, 7, 1, SELECTFROMLIST, TEXT, "Pump 1"};
	states[7]  = {6, 8, 1, SELECTFROMLIST, TEXT, "Pump 2"};
	states[8]  = {7, 9, 1, SELECTFROMLIST, TEXT, "Pump 3"};
	states[9]  = {8, 6, 1, SELECTFROMLIST, TEXT, "Pump 4"};

}


void loop()
{
	uint8_t next_state;
	getInput();
	next_state = states[current_state].button[last_button];
	if (next_state != 0 && next_state < 200)
    {
      current_state = next_state;
      printScreen();
    }
    /*
	else if (next_state == CHANGEVALUE)  changeValue();
    else if (next_state == INFUSEVOLUME) infuseVolume();
    else if (next_state == INFUSETIME) infuseTime();
    else if (next_state == REFILLVOLUME) refillVolume();
    else if (next_state == REFILLFULL) refillFull();
    else if (next_state == CYCLEMODE) pumpContinuously();
    else if (next_state == FIRMWAREINFO) printFirmwareInfo();
	*/
}


void getInput()
{
	uint8_t button = NA;
	if (Serial.available()>0)
	{
		uint8_t x = Serial.read();
		if (x == "A") button = OP_A;
		if (x == "B") button = OP_B;
		if (x == "C") button = OP_C;
		if (x == "D") button = OP_D;
		else button = NA;
	}
	if(button == NA)
	{
		last_button = NA;
	}
	else if ( button != last_button)
	{
		last_button = button;
	}
}

void printScreen()
{

  Serial.println("-------------------------");

  if (states[current_state].type == TEXT || states[current_state].type == LIST)
  {
    Serial.print(F("> "));
    Serial.println(state[current_state].text);

    if (states[current_state].button[OP_B] != 0 && states[current_state].button[OP_B] < 200)
    {

      Serial.print(F("  "));
      Serial.println(states[states[current_state].button[OP_B]].text);
    }
  }
  else
  {
    calculateActualValue(current_state);
    float value = getValue(current_state);


    Serial.print(states[current_state].text);
    Serial.println(value);
    printUnits();
  }
}

void calculateActualValue(uint8_t _current_state)
{}
float getValue(uint8_t _current_state)
{}
void printUnits()
{

}

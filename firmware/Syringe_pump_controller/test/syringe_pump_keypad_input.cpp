#include <Arduino.h>
#include <Wire.h>
#include <PCF8574.h>
#include <LiquidCrystal_I2C.h>
#include <I2CKeyPad.h>

#define KEYPAD_ADDR 0x24  // Dirección I2C del PCF8574A
#define GPIO_ADDR 0x20 // Dirección I2C del PCF8574A
#define LCD_ADDR 0x27      // Dirección I2C de la pantalla LCD

#define RUN 1
#define STOP 0


I2CKeyPad keypad(KEYPAD_ADDR);
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

uint32_t start, stop;
uint32_t lastKeyPressed = 0;

bool key_change = false;

const char keyMap[16] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};


uint8_t	estado_pantalla = 0;
uint8_t estado_motor = STOP;
char key;


char getKey();
void processKey(char _key);
void keyChanged();


void setup()
{
    Wire.begin();
    lcd.init();
	lcd.backlight();
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("WenzelLab ");
 	if (keypad.begin() == false)
  	{
    	Serial.println("\nERROR: cannot communicate to keypad.\nPlease reboot.\n");
    	while(1);
  	}

		//  NOTE: PCF8574 will generate an interrupt on key press and release.
	pinMode(5, INPUT_PULLUP);
	attachInterrupt(5, keyChanged, FALLING);
	key_change = false;

	Serial.begin(9600);
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



char getKey()
{
//	uint32_t now = millis();
	char keys[] = "123A456B789C*0#DNF";  //  N = NoKey, F = Fail

//	if (now - lastKeyPressed >= 200) // deboucing delay
//	{
//		lastKeyPressed = now;
//		start = micros();
		uint8_t index = keypad.getKey();
//		stop = micros();
		if(keys[index] != 'N')
		{
			Serial.print("Tecla presionada: ");
			Serial.println(keys[index]);
			//lcd.clear();
			//delay(100);
			//lcd.setCursor(1, 0);
			//lcd.print("Tecla: ");
			//lcd.print(keys[index]);
		}
		return keys[index];
	//}
}

void processKey(char _key)
{
	if(_key != 'N')
	{
		lcd.setCursor(0, 1);
		lcd.print("Tecla: ");
		lcd.print(_key);
	}

}

void keyChanged()
{
	key_change = true;
}

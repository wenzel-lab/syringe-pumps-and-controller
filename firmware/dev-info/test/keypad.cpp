
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





#define ID "SPC-01"
#define VERSION "SP-v1.1"
#define RESPONSE_OK     		"OK"
#define RESPONSE_ERROR  		"ERROR"
#define RESPONSE_ERROR_JSON  	"ERROR_INPUT_JSON"
#define RESPONSE_ERROR_CMD 		"ERROR_CMD"

//MQTT
#define PUBLISH_TOPIC  	"wl/sp/tx"
#define SUBSCRIBE_TOPIC "wl/sp/rx"

#define mqtt_server 	"35.223.234.244"
#define mqtt_user 		"iowlabs"
#define mqtt_password 	"!iow_woi!"
#define mqtt_port 1883


// Configuración de la red Wi-Fi
const char* ssid = "Taller.1";
const char* password = "@Salvo20";

#define STEP_2_mL 25

// Configuración del cliente Wi-Fi y MQTT
WiFiClient espClient;
PubSubClient client(espClient);

PCA9555 ioport(0x20);
LiquidCrystal_I2C lcd(0x27,16,2);  //

//Task to handel mqtt rcv commands
TaskHandle_t Task1;

//cmd
const char* cmd;
const char* rcvd_id;
const char* response = RESPONSE_OK;

//KEYPAD
const uint8_t KEYPAD_ADDRESS = 0x24;
I2CKeyPad keyPad(KEYPAD_ADDRESS);
char keys[] = "123A456B789C*0#DNF";  //  N = NoKey, F = Fail (e.g. > 1 keys pressed)


// Pines del motor stepper
int EN_PINS[] 	= {15,11,0,4};
int STEP_PINS[] = {18,16,32,25};
int DIR_PINS[]  = {17,4,33,26};

// volatile for IRQ var
volatile bool keyChange = false;
String inputNumber = ""; // Para almacenar el número ingresado
int channel = 2;
int steps = 0;
bool bussy_mqtt = false;

void moveStepper(int _steps, int _ch);
void keyChanged();
void setup_wifi();
void reconnect();
void processCmd(byte* payload, unsigned int length);
void publishMqtt(char *payload);
void publishResponse();
void callback(char* topic, byte* payload, unsigned int length);
void measurePolling();
void Task1code(void *pvParameters);

void setup()
{
	Serial.begin(115200);

	setup_wifi();
	client.setServer(mqtt_server, mqtt_port);
	client.setCallback(callback);

  	//  NOTE: PCF8574 will generate an interrupt on key press and release.
  	pinMode(5, INPUT_PULLUP);
  	attachInterrupt(5, keyChanged, FALLING);
  	keyChange = false;

	Wire.begin();

	Wire.setClock(100000);
	if (keyPad.begin() == false)
	{
		Serial.println("\nERROR: cannot communicate to keypad.\nPlease reboot.\n");
	}

	ioport.begin();

	//Define pin modes
	for(int i = 0;i<4;i++)
	{
		ioport.pinMode(EN_PINS[i], OUTPUT);
		ioport.digitalWrite(EN_PINS[i], HIGH);

		pinMode(DIR_PINS[i],OUTPUT);
		pinMode(STEP_PINS[i],OUTPUT);
	}


	lcd.init();
	lcd.backlight();
	// Escribimos el Mensaje en el LCD.
	lcd.setCursor(0, 0);
	lcd.print("SP - ctrl");

	delay(100);
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

	measurePolling();
}


void loop()
{
	if (keyChange)
	{
    	uint8_t index = keyPad.getKey();
    	keyChange = false; // Reinicia la bandera de interrupción

    	if (index != 16)
		{ // Si se detecta una tecla
      		char key = keys[index];

      		if (key >= '0' && key <= '9')
			{
        		// Concatenar el número ingresado
        		inputNumber += key;
        		lcd.setCursor(0, 1);
        		lcd.print("ml: ");
        		lcd.print(inputNumber);
      		}
			else if (key == 'D')
			{
        		// Ejecutar el movimiento del motor con los pasos ingresados
        		steps = inputNumber.toInt();

				lcd.setCursor(0, 0);
        		Serial.print("Ejecutando ");

				Serial.print(steps);
        		Serial.println(" pasos.");

        		lcd.setCursor(0, 1);
        		lcd.print("Moviendo...      ");
        		moveStepper(steps,channel);

        		// Reiniciar el número ingresado
        		inputNumber = "";
        		lcd.setCursor(0, 1);
        		lcd.print("Listo           ");
      		}
			else if (key == '*')
			{
        		// Limpiar el número ingresado
        		inputNumber = "";
        		lcd.setCursor(0, 1);
        		lcd.print("                "); // Borrar línea
        		lcd.setCursor(0, 0);
        		lcd.print("Ingrese ml:");
      		}
    	}
  	}
}

void Task1code( void * pvParameters )
{
 	Serial.print("Task1 running on core ");
 	Serial.println(xPortGetCoreID());

	for(;;)
	{
		if (bussy_mqtt == false)
		{
			if (!client.connected())
			{
	    		reconnect();
	  		}
		}

		client.loop();

    	vTaskDelay(1);  // Cede el control por un tick del sistema
  	}
}

// Función de callback para manejar mensajes recibidos
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

void publishMqtt(char *payload)
{
	bussy_mqtt = true;
	Serial.print("sending data by mqtt . . .");
	client.publish(PUBLISH_TOPIC, payload , true);
	Serial.println(" done");
	bussy_mqtt = false;
}

void setup_wifi()
{
	delay(10);
	Serial.println("Conectando a Wi-Fi...");
	WiFi.begin(ssid, password);
 	while (WiFi.status() != WL_CONNECTED)
	{
    	delay(1000);
    	Serial.print(".");
  	}
  	Serial.println("\nConexión Wi-Fi establecida. Dirección IP: ");
  	Serial.println(WiFi.localIP());
}


void reconnect()
{
  	while (!client.connected())
	{
    	Serial.print("Conectando al broker MQTT...");
    	if (client.connect("ESP32Client", mqtt_user, mqtt_password))
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
    	}
  	}
}

/*
  this function only send a respone type of data.
  used for communication.
*/
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


/*
  Parse the received json message and procces the recived commands
*/
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
		else if(strcmp(cmd,"mov")==0)
      	{
			channel  = arg_js["ch"];
			steps    = arg_js["ml"];
			moveStepper(steps, channel);
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

// Función para mover el motor
void moveStepper(int _steps, int _ch)
{
	if(_ch < 0 or _ch > 3)return;

	ioport.digitalWrite(EN_PINS[_ch], LOW);
	digitalWrite(DIR_PINS[_ch], HIGH); // Configurar dirección (puedes ajustar según necesites)

	for (int i = 0; i < _steps; i++)
	{
    	digitalWrite(STEP_PINS[_ch], HIGH);
    	delayMicroseconds(500); // Ajusta según la velocidad deseada
    	digitalWrite(STEP_PINS[_ch], LOW);
    	delayMicroseconds(500); // Ajusta según la velocidad deseada
  	}

  	ioport.digitalWrite(EN_PINS[_ch], HIGH); // Deshabilitar el motor
}

void keyChanged()
{
	keyChange = true;
}


void measurePolling()
{
	for (uint32_t clock = 100000; clock <= 800000; clock += 100000)
	{
		Wire.setClock(clock);
		for (int i = 0; i < 1; i++)
		{
			//  reference time for keyPressed check UNO ~
			uint32_t start = micros();
			uint8_t index = keyPad.isPressed();
			uint32_t stop = micros();

      		Serial.print(clock);
      		Serial.print("\t");
      		Serial.print(index);
      		Serial.print("\t");
      		Serial.print(keys[index]);
      		Serial.print("\t");
      		Serial.println(stop - start);
      		delay(10);
		}
	}
}


//  -- END OF FILE --

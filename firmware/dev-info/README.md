# Instructions


## 1. Adding ESP32 board to arduino interface
The first step to upload the code is to add the ESP32 board to the board manager in the Arduino IDE. If it's already added, skip to the next step.

1. Go to `File > Preferences`. In the window that appears, add the following link to the `Additional Boards Manager URLs` section:
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

In version 2.0 or higher, this can be done directly through the board manager. Go to `Tools > Boards > Board Manager`, search for `ESP32` by Espressif System, and then click `Install`.

2. Select the ESP32 Board:

Now, we need to select the correct board. To do this, go to `Tools > Board > esp32`.

You can select any board compatible with the card you're using.
It's recommended to use one such as:
- `ESP32 DEV MODULE`
- `DOIT ESP32 DEVKIT V1`


3.  Connect the ESP32 to Your Computer:

Use a USB cable to connect the ESP32 to your computer.
Select the correct port in the Arduino IDE under `Tools > Port`.


## 2. Adding Used libraries
The code uses several libraries. However, all of these are available from the official repository of Arduino. To add a library, go to `Tools > Manage Libraries...` search for the library in the search bar and click `Install`.

The libraries used by the code are listed below.

Library | author | description
--------|-------|-------------
I2CKeyPad| robtillaart 	| Handle i2c kepad interface
PCF8574 library | xreef | Control the i2c to GPIO expander
LiquidCrystal_I2C | marcoschwartz | LCD I2C interface.
ArduinoJson 	|  bblanchon | Json utilites
PubSubClient 	| knolleary | Mqtt client
ContinuousStepper | bblanchon | Api to control the stepper motor by RPMs.



## 3. Upload the code
Now, with the setup ready and the libraries installed, we can load the code. The code and its source files were developed in Platformio. The source files are located in the following repository:
- [firmware
/Syringe_pump_controller](https://github.com/wenzel-lab/syringe-pumps-and-controller/tree/docu-v2/firmware/Syringe_pump_controller)

However, the latest available version of the code has a version compatible with the Arduino IDE. This `.ino` file can be found at the following link.

- [firmware
/Syringe_pump_controller_arduino](https://github.com/wenzel-lab/syringe-pumps-and-controller/tree/docu-v2/firmware/Syringe_pump_controller_arduino)

Once the code is downloaded, you can open it with your IDE, build it, and then upload it to the board.

Important: The `versions.txt` file aviable in the repository provides details of the current code versions. Make sure the version loaded is the latest available and documented. Once the code is loaded, you can see the current version in the first message displayed on the serial console when the firmware is started.

# Internet control usage.

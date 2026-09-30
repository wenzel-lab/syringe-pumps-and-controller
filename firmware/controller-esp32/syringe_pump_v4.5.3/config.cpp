#include "config.h"
#ifndef FIRMWARE_VERSION
#error "FIRMWARE_VERSION must be defined in config.h"
#endif

// Pin Definitions
const int EN_PINS[] = {15, 11, 7, 3};    // Enable pins for motors A-D
const int STEP_PINS[] = {4, 27, 26, 33}; // Step pins for motors A-D
const int DIR_PINS[] = {23, 5, 25, 32};  // Direction pins for motors A-D
const int MS1_PINS[] = {14, 10, 6, 2};   // MS1 pins for motors A-D
const int MS2_PINS[] = {13, 9, 5, 1};    // MS2 pins for motors A-D
const char MOTORS_NAMES[] = {'A', 'B', 'C', 'D'};  // Motor identifiers

// Microstep configurations for TMC2209 drivers
const MicrostepConfig MICROSTEP_CONFIGS[] = {
    {LOW,  LOW},   // 1/8
    {HIGH, HIGH},  // 1/16
    {LOW,  HIGH},  // 1/32
    {HIGH, LOW}    // 1/64
};

// Keypad configuration
const char keyMap[16] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};

// UI navigation state
volatile bool key_change = false;
String num_input = "";
uint8_t current_state = 0;
uint8_t next_state = 0;
char actual_motor = MOTOR_A;
char key = 0;
const char* response = "OK";


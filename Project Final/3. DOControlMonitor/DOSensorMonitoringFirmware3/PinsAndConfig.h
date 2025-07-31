#pragma once
#include "Arduino.h"

#define IS_ARDUINO_BOARD 0
#define ENABLE_FIREBASE 0

#if IS_ARDUINO_BOARD
#define ENABLE_MODULE_EEPROM_LIB
#else
#define ENABLE_MODULE_FIREBASE_HANDLER
#define ENABLE_MODULE_EEPROM_LIB_ESP8266
#define ENABLE_MODULE_DATETIME_NTP
#endif
#define ENABLE_MODULE_PID

#define DO_SENSOR_PIN A0

#if IS_ARDUINO_BOARD
#define MODBUS_RO_PIN 13  // RX
#define MODBUS_RE_PIN 11  // EN
#define MODBUS_DE_PIN 11  // EN
#define MODBUS_DI_PIN 12  // TX
#define PWM_OUTPUT_PIN 5
#define DO_SENSOR_VREF_MV 5000
#else
#define MODBUS_RO_PIN D13  // RX
#define MODBUS_RE_PIN D11  // EN
#define MODBUS_DE_PIN D11  // EN
#define MODBUS_DI_PIN D12  // TX
#define PWM_OUTPUT_PIN D4
#define DO_SENSOR_VREF_MV 3300
#endif

#ifdef UNO_WEMOS_D1_R1_PIN_DEFINITION
#define LED_BUILTIN 2
static const uint8_t D0 = 3;
static const uint8_t D1 = 1;
static const uint8_t D2 = 16;
static const uint8_t D3 = 5;   // Safe to Use
static const uint8_t D4 = 4;   // Safe to Use
static const uint8_t D5 = 14;  // Safe to Use
static const uint8_t D6 = 12;  // Safe to Use
static const uint8_t D7 = 13;  // Safe to Use
static const uint8_t D8 = 0;
static const uint8_t D9 = 2;
static const uint8_t D10 = 15;
static const uint8_t D11 = 13;  // Safe to Use
static const uint8_t D12 = 12;  // Safe to Use
static const uint8_t D13 = 14;  // Safe to Use
static const uint8_t D14 = 4;   // Safe to Use
static const uint8_t D15 = 5;   // Safe to Use
#endif
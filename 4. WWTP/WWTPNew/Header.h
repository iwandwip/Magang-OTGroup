#pragma once

const uint8_t PIN_RADAR_SENSOR = 2;
const uint8_t PIN_AC_VOLTAGE = A0;
const uint8_t PIN_FILLING_PUMP = 9;   // In6 - filling tank pump
const uint8_t PIN_DOSING_PUMP = 11;   // In5 - dosing pump
const uint8_t PIN_TRANSFER_PUMP = 8;  // In4 - transfer pump

const uint8_t EEPROM_ADDR_DELAY1 = 0;
const uint8_t EEPROM_ADDR_DELAY2 = 1;

const unsigned long FILLING_TIMEOUT = 250000;   // 250 seconds
const unsigned long TRANSFER_TIMEOUT = 380000;  // 380 seconds
const uint16_t VOLTAGE_SAMPLES = 500;
const uint16_t RADAR_SAMPLES = 200;
const float VOLTAGE_THRESHOLD = 500.0;
const unsigned long VERIFICATION_TIME = 15000;
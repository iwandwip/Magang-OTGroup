#pragma once

const uint8_t PIN_RADAR_SENSOR = 2;
const uint8_t PIN_AC_VOLTAGE = A1;    // A0
const uint8_t PIN_FILLING_PUMP = 9;   // In6 - filling tank pump
const uint8_t PIN_DOSING_PUMP = 11;   // In5 - dosing pump
const uint8_t PIN_TRANSFER_PUMP = 8;  // In4 - transfer pump

const uint8_t EEPROM_ADDR_DELAY1 = 0;
const uint8_t EEPROM_ADDR_DELAY2 = 0;

// const uint8_t EEPROM_ADDR_STATE = 0;
// const uint8_t EEPROM_ADDR_FILLING_TIME = 4;
// const uint8_t EEPROM_ADDR_TRANSFER_TIME = 8;

const uint32_t FILLING_TIMEOUT_S = 250;                       // 250 seconds
const uint32_t TRANSFER_TIMEOUT_S = 210;                      // 210 seconds
const uint32_t FILLING_TIMEOUT = FILLING_TIMEOUT_S * 1000;    // ms
const uint32_t TRANSFER_TIMEOUT = TRANSFER_TIMEOUT_S * 1000;  // ms

const uint16_t VOLTAGE_SAMPLES = 500;
const uint16_t RADAR_SAMPLES = 200;
const float VOLTAGE_THRESHOLD = 500.0;
const int ADC_THRESHOLD = 950;
const uint32_t VERIFICATION_TIME = 5;
const uint32_t VERIFICATION_TIME_MS = VERIFICATION_TIME * 1000;
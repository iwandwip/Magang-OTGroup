#define ENABLE_MODULE_EEPROM_LIB

#include "Kinematrix.h"

#include "ModbusSlave.h"
#include "SoftwareSerial.h"

const int pinRo = 10;  // RX
const int pinRe = 11;  // EN
const int pinDe = 11;  // EN
const int pinDi = 12;  // TX

SoftwareSerial modbus(pinRo, pinDi);
Modbus slave(modbus, 1, pinRe);
EEPROMLib eeprom;

enum HoldingRegisterIndex {
  ADC_REG,
  VOLT_REG,
  DO_REG,
  PERCENT_REG,
  CALIBRATION_REG,
  HOLDING_REGISTER_LEN  // Length
};

constexpr uint16_t ADDRESS_OFFSET = 1;
enum HoldingRegisterAddress {
  ADC_ADDRESS = ADC_REG * 2 + ADDRESS_OFFSET,
  VOLT_ADDRESS = VOLT_REG * 2 + ADDRESS_OFFSET,
  DO_ADDRESS = DO_REG * 2 + ADDRESS_OFFSET,
  PERCENT_ADDRESS = PERCENT_REG * 2 + ADDRESS_OFFSET,
  CALIBRATION_ADDRESS = CALIBRATION_REG * 2 + ADDRESS_OFFSET,
  HOLDING_REGISTER_ADDRESS_LEN = HOLDING_REGISTER_LEN * 2 + ADDRESS_OFFSET
};

enum CoinStateDataIndex {
  POWER_BUTTON,   // 0
  PUMP_BUTTON,    // 1
  COIL_STATE_LEN  // Length
};

constexpr uint16_t STATE_OFFSET = 1;
enum CoinStateDataAddress {
  POWER_BUTTON_ADDRESS = POWER_BUTTON + STATE_OFFSET,
  PUMP_BUTTON_ADDRESS = PUMP_BUTTON + STATE_OFFSET,
  COIL_STATE_ADDRESS_LEN = COIL_STATE_LEN + STATE_OFFSET
};

uint8_t coilState[COIL_STATE_LEN];
float holdingRegister[HOLDING_REGISTER_LEN];

void setup() {
  slave.cbVector[CB_READ_COILS] = readCoils;
  slave.cbVector[CB_READ_DISCRETE_INPUTS] = readCoils;
  slave.cbVector[CB_WRITE_COILS] = writeCoils;
  slave.cbVector[CB_READ_INPUT_REGISTERS] = readInputRegister;
  slave.cbVector[CB_READ_HOLDING_REGISTERS] = readHoldingRegister;
  slave.cbVector[CB_WRITE_HOLDING_REGISTERS] = writeHoldingRegister;

  holdingRegister[CALIBRATION_REG] = eeprom.readFloat(200);  // 200 - 999
  // coilState[POWER_BUTTON] = eeprom.readInt(0);               // 0 - 199
  // coilState[PUMP_BUTTON] = eeprom.readInt(2);

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);

  modbus.begin(9600);
  slave.begin(9600);
}

void loop() {
  static uint32_t timerHoldingRegister;
  if (millis() - timerHoldingRegister >= 1000) {
    timerHoldingRegister = millis();

    holdingRegister[ADC_REG] = random(800, 1023);
    holdingRegister[VOLT_REG] = random(30, 50) * 0.1;
    holdingRegister[DO_REG] = random(0, 200) * 0.1;
    holdingRegister[PERCENT_REG] = random(0, 70);
  }

  digitalWrite(LED_BUILTIN, coilState[POWER_BUTTON]);

  slave.poll();
}
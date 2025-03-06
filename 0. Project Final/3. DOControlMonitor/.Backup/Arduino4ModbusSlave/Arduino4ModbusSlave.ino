#include <ModbusSlave.h>
#include <SoftwareSerial.h>

const int pinRo = 10;
const int pinRe = 11;
const int pinDe = 11;
const int pinDi = 12;

SoftwareSerial modbusSerial(pinRo, pinDi);

Modbus slave(modbusSerial, 1, pinRe);

// Buffer untuk menyimpan data Holding Registers
uint16_t holdingRegisters[10];

// Fungsi callback untuk membaca holding registers
uint8_t readHoldingRegisters(uint8_t fc, uint16_t address, uint16_t length, void* data) {
  uint16_t* buffer = (uint16_t*)data;
  for (uint16_t i = 0; i < length; i++) {
    buffer[i] = holdingRegisters[address + i];
  }
  return STATUS_OK;
}

void setup() {
  Serial.begin(9600);  // Inisialisasi komunikasi serial
  slave.begin(9600);   // Inisialisasi Modbus pada baud rate 9600

  // Set callback untuk fungsi membaca Holding Registers
  slave.cbVector[CB_READ_HOLDING_REGISTERS] = readHoldingRegisters;
}

void loop() {
  slave.poll();  // Menangani komunikasi Modbus

  // Contoh data sensor yang dikonversi ke float
  float sensor1 = 123.45;  // Data sensor 1
  float sensor2 = 678.90;  // Data sensor 2
  float sensor3 = 345.67;  // Data sensor 3

  // Konversi float ke register (2 x 16-bit per float)
  uint16_t* sensor1Registers = (uint16_t*)&sensor1;
  uint16_t* sensor2Registers = (uint16_t*)&sensor2;
  uint16_t* sensor3Registers = (uint16_t*)&sensor3;

  // Simpan data float ke holdingRegisters
  holdingRegisters[1] = sensor1Registers[0];  // MSB dari sensor1
  holdingRegisters[2] = sensor1Registers[1];  // LSB dari sensor1
  holdingRegisters[3] = sensor2Registers[0];  // MSB dari sensor2
  holdingRegisters[4] = sensor2Registers[1];  // LSB dari sensor2
  holdingRegisters[5] = sensor3Registers[0];  // MSB dari sensor3
  holdingRegisters[6] = sensor3Registers[1];  // LSB dari sensor3

  // Delay untuk menghindari polling yang terlalu cepat
  delay(1000);
}

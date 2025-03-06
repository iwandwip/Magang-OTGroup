#include "Vending.h"

VendingControllerBridge::VendingControllerBridge(Stream* commStream, IDebugger* debugger)
  : ModbusBase(commStream, debugger) {}

bool VendingControllerBridge::writeBytes(byte* data, int length) {
  if (data == nullptr || length <= 0 || !isCommValid()) return false;

  debug->print("| send: ");
  for (int i = 0; i < length; i++) {
    comm->write(data[i]);
    debug->printHex(data[i]);
  }
  debug->println("");
  return true;
}

bool VendingControllerBridge::writeHexString(String hexString) {
  byte buffer[32];
  int length;
  if (!ModbusUtils::hexStringToBytes(hexString, buffer, sizeof(buffer), &length)) {
    debug->println("Error converting hex string");
    return false;
  }
  return writeBytes(buffer, length);
}

String VendingControllerBridge::generateModbusFrame(uint8_t address) {
  byte data[20];
  String result = "";
  data[0] = 0x01;                 // Device address
  data[1] = 0x05;                 // Function code
  data[2] = address;              // Parameter address
  data[3] = 0x03;                 // Operand 1
  data[4] = 0x01;                 // Operand 2
  for (int i = 5; i < 18; i++) {  // Padding 14 bytes
    data[i] = 0x00;
  }
  uint16_t crc = ModbusUtils::calculateCRC16(data, 18);
  data[19] = crc >> 8;    // High byte
  data[18] = crc & 0xFF;  // Low byte

  for (int i = 0; i < 20; i++) {
    if (data[i] < 0x10) {
      result += "0";
    }
    result += String(data[i], HEX);
  }
  return result;
}
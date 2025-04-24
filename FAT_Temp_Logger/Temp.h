#include <OneWire.h>

#define MAX_SENSORS 4

float Temp[4] = {0,0,0,0};
const uint8_t sensorPins[MAX_SENSORS] = { 4, 27, 26, 25 };  // Define your GPIO pins
OneWire oneWire[MAX_SENSORS] = {
  OneWire(sensorPins[0]),
  OneWire(sensorPins[1]),
  OneWire(sensorPins[2]),
  OneWire(sensorPins[3])
};

uint8_t sensorAddresses[MAX_SENSORS][8];
bool sensorFound[MAX_SENSORS] = { false, false, false, false };

// Function to print address
void printAddress(uint8_t *address) {
  for (uint8_t i = 0; i < 8; i++) {
    if (address[i] < 16) Serial.print("0");
    Serial.print(address[i], HEX);
  }
  Serial.println();
}

// Find sensors — one per pin
void findSensors() {
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    oneWire[i].reset_search();
    if (oneWire[i].search(sensorAddresses[i])) {
      if (OneWire::crc8(sensorAddresses[i], 7) == sensorAddresses[i][7]) {
        sensorFound[i] = true;
        Serial.print("Sensor ");
        Serial.print(i + 1);
        Serial.print(" on pin ");
        Serial.print(sensorPins[i]);
        Serial.print(" Address: ");
        printAddress(sensorAddresses[i]);
      } else {
        Serial.print("CRC error on sensor at pin ");
        Serial.println(sensorPins[i]);
      }
    } else {
      Serial.print("No sensor found on pin ");
      Serial.println(sensorPins[i]);
    }
  }
}

// Request temperature and read result
float readTemperature(OneWire &ds, uint8_t *addr) {
  byte data[9];

  ds.reset();
  ds.select(addr);
  ds.write(0x44);  // Start temperature conversion
  delay(750);      // Max conversion time

  ds.reset();
  ds.select(addr);
  ds.write(0xBE);  // Read scratchpad

  for (byte i = 0; i < 9; i++) {
    data[i] = ds.read();
  }

  // CRC check
  if (OneWire::crc8(data, 8) != data[8]) {
    Serial.println("CRC error during temperature read");
    return NAN;
  }

  int16_t raw = (data[1] << 8) | data[0];
  return raw / 16.0;
}

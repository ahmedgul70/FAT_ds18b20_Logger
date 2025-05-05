#include "Temp.h"

float Temp[MAX_SENSORS] = {0, 0, 0, 0};
OneWire oneWire[MAX_SENSORS] = {
    OneWire(sensorPins[0]),
    OneWire(sensorPins[1]),
    OneWire(sensorPins[2]),
    OneWire(sensorPins[3])};
uint8_t sensorAddresses[MAX_SENSORS][8];
bool sensorFound[MAX_SENSORS] = {false, false, false, false};

void printAddress(uint8_t *address)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (address[i] < 16)
      Serial.print("0");
    Serial.print(address[i], HEX);
  }
  Serial.println();
}

void findSensors()
{
  for (uint8_t i = 0; i < MAX_SENSORS; i++)
  {
    oneWire[i].reset_search();
    if (oneWire[i].search(sensorAddresses[i]))
    {
      if (OneWire::crc8(sensorAddresses[i], 7) == sensorAddresses[i][7])
      {
        sensorFound[i] = true;
        Serial.print(F("Sensor "));
        Serial.print(i + 1);
        Serial.print(F(" on pin "));
        Serial.print(sensorPins[i]);
        Serial.print(F(" Address: "));
        printAddress(sensorAddresses[i]);
      }
      else
      {
        Serial.print(F("CRC error on sensor at pin "));
        Serial.println(sensorPins[i]);
      }
    }
    else
    {
      Serial.print(F("No sensor found on pin "));
      Serial.println(sensorPins[i]);
    }
  }
}

float readTemperature(OneWire &ds, uint8_t *addr)
{
  uint8_t data[9];
  ds.reset();
  ds.select(addr);
  ds.write(0x44);
  delay(750);
  ds.reset();
  ds.select(addr);
  ds.write(0xBE);
  for (uint8_t i = 0; i < 9; i++)
  {
    data[i] = ds.read();
  }
  if (OneWire::crc8(data, 8) != data[8])
  {
    Serial.println(F("CRC error during temperature read"));
    return NAN;
  }
  int16_t raw = (data[1] << 8) | data[0];
  return raw / 16.0;
}

void getReadings()
{
  for (uint8_t i = 0; i < MAX_SENSORS; i++)
  {
    if (sensorFound[i])
    {
      Temp[i] = readTemperature(oneWire[i], sensorAddresses[i]);
    }
  }
}
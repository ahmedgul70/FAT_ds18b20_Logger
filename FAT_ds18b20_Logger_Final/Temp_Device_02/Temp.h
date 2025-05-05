#include <OneWire.h>

#define MAX_SENSORS 4

extern float Temp[MAX_SENSORS];
const uint8_t sensorPins[MAX_SENSORS] = {4, 27, 26, 25};
extern OneWire oneWire[MAX_SENSORS];
extern uint8_t sensorAddresses[MAX_SENSORS][8];
extern bool sensorFound[MAX_SENSORS];

void printAddress(uint8_t *address);
void findSensors();
float readTemperature(OneWire &ds, uint8_t *addr);
void getReadings();
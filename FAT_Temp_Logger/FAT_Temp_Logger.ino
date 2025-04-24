#include "Temp.h"
#include "SD_module.h"
#include "Arduino.h"
#include <Wire.h>
#include "RTClib.h"

#define SD_Module_CS 5
RTC_DS3231 rtc;

float temp1 = 0.0;
float temp2 = 27.7;
float temp3 = 99.9;
float temp4 = -127;

unsigned long pmillis = 0;
String timestamp;  // fileName

void setup() {
  Serial.begin(115200);
  Serial.println("Serial OK.");
  Wire.begin();  // Uses default SDA (21), SCL (22) on ESP32

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }
  // Set time to: YYYY, MM, DD, HH, MM, SS
  // rtc.adjust(DateTime(2025, 4, 21, 2, 23, 0));
  scanRTC();

  // Start the DS18B20 sensors
  findSensors();

  if (!SD.begin(SD_Module_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }
  Serial.println("Card initialized.");
  SDinfo();

  DateTime now = rtc.now();
  // Create a timestamp string
  timestamp = "/" + String(now.day()) + "-" + String(now.month()) + "-" + String(now.year()) + "T" + String(now.hour()) + "-" + String(now.minute()) + "-" + String(now.second()) + ".csv";
  Serial.println("File to open: " + timestamp);


  // getReadings();
  // CardInit();
  // logSDCard();
  // readSDcard();
  // delay(5000);
}

void loop() {
  // if (millis() - pmillis > 2000) {
  //   getReadings();
  //   // scanRTC();
  //   logSDCard();
  //   pmillis = millis();
  // }
}

void CardInit() {
  // Create or open CSV file
  // File myFile = SD.open("/data_log.csv", FILE_WRITE);
  File myFile = SD.open(timestamp.c_str(), FILE_WRITE);
  if (myFile) {
    // Write headers only once (if new file)
    if (myFile.size() == 0) {
      myFile.println("Timestamp,Temp1,Temp2,Temp3,Temp4");
    }

    // // Example data (use your actual sensor readings)
    // float temperature = 25.4;
    // float humidity = 60.2;

    // // Format data as CSV line
    // myFile.print(timestamp);
    // myFile.print(",");
    // myFile.print(temperature);
    // myFile.print(",");
    // myFile.println(humidity);

    myFile.close();  // Always close the file!
    Serial.println("✅ Data written to CSV.");
  } else {
    Serial.println("❌ Error opening CSV file!");
  }
}

void logSDCard() {
  File myFile = SD.open(timestamp.c_str(), FILE_WRITE);
  if (myFile) {
    // Example data (use your actual sensor readings)
    DateTime now = rtc.now();
    String Tstamp = "/" + String(now.day()) + "-" + String(now.month()) + "-" + String(now.year()) + "T" + String(now.hour()) + "-" + String(now.minute()) + "-" + String(now.second()) + ".csv";

    // Format data as CSV line
    myFile.print(Tstamp);
    myFile.print(",");
    myFile.print(Temp[0]);
    myFile.print(",");
    myFile.print(Temp[1]);
    myFile.print(",");
    myFile.print(Temp[2]);
    myFile.print(",");
    myFile.println(Temp[3]);

    myFile.close();  // Always close the file!
    Serial.println("✅ Data written to CSV.");
  } else {
    Serial.println("❌ Error opening CSV file!");
  }
}

void readSDcard() {
  // Open CSV file for reading
  // File myFile = SD.open("/data_log.csv", FILE_READ);
  File myFile = SD.open(timestamp.c_str(), FILE_READ);

  if (myFile) {
    Serial.println("📂 Reading /" + timestamp + ":\n");

    while (myFile.available()) {
      String line = myFile.readStringUntil('\n');  // Read line by line
      Serial.println(line);
    }

    myFile.close();
    Serial.println("\n✅ Done reading file.");
  } else {
    Serial.println("❌ Failed to open /" + timestamp + ".csv for reading.");
  }
}

void scanRTC() {
  DateTime now = rtc.now();

  Serial.print("Timestamp: ");
  Serial.println(now.timestamp());

  Serial.print("Date: ");
  Serial.print(now.year());
  Serial.print('/');
  Serial.print(now.month());
  Serial.print('/');
  Serial.print(now.day());

  Serial.print("  Time: ");
  Serial.print(now.hour());
  Serial.print(':');
  Serial.print(now.minute());
  Serial.print(':');
  Serial.println(now.second());
}

void getReadings() {
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    if (sensorFound[i]) {
      Temp[i] = readTemperature(oneWire[i], sensorAddresses[i]);
      Serial.print("Temperature from sensor ");
      Serial.print(i + 1);
      Serial.print(" (pin ");
      Serial.print(sensorPins[i]);
      Serial.print("): ");
      Serial.print(Temp[i]);
      Serial.println(" °C");
    }
  }
  Serial.println();
}

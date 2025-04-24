#include <Wire.h>
#include "RTClib.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

RTC_DS3231 rtc;

// float temp1 = 0.0;
// float temp2 = 27.7;
// float temp3 = 99.9;
// float temp4 = -127;

unsigned long pmillis = 0;
String timestamp;  // fileName on SD card

// Replace with your network credentials
const char* ssid = "RES_ESP32";
const char* password = "12345678";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Initialize SD Card
void initSDCard() {
  if (!SD.begin(5)) {
    Serial.println("Card Mount Failed");
    return;
  }
  Serial.println("Card initialized.");

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) Serial.println("MMC");
  else if (cardType == CARD_SD) Serial.println("SDSC");
  else if (cardType == CARD_SDHC) Serial.println("SDHC");
  else Serial.println("UNKNOWN");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  // Connect to Wi-Fi network with SSID and password
  // Serial.print("Setting AP (Access Point)…");
  // // Remove the password parameter, if you want the AP (Access Point) to be open
  // WiFi.softAP(ssid, password);
  // // Serial.println(WiFi.localIP());
  // Serial.println(WiFi.softAPIP());
}

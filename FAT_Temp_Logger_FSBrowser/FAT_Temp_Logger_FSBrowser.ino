#include "Temp.h"
#include "SD_module.h"
#include "Def.h"

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
  initSDCard();
  initWiFi();

  DateTime now = rtc.now();
  // Create a timestamp string
  timestamp = "/" + String(now.day()) + "-" + String(now.month()) + "-" + String(now.year()) + "T" + String(now.hour()) + "-" + String(now.minute()) + "-" + String(now.second()) + ".csv";
  Serial.println("File to open: " + timestamp);

  // getReadings();
  CardInit();

  // Route to list CSV files
  server.on("/listcsv", HTTP_GET, [](AsyncWebServerRequest* request) {
    File root = SD.open("/");
    if (!root || !root.isDirectory()) {
      request->send(500, "text/plain", "Failed to open SD card root directory");
      return;
    }

    File file = root.openNextFile();
    String html = "<html><body><h2>CSV Files on SD Card</h2><ul>";
    int count = 0;

    while (file) {
      String filename = file.name();
      if (!file.isDirectory() && filename.endsWith(".csv")) {
        // html += "<li><a href='/viewcsv?file=" + filename + "'>" + filename + "</a></li>";
        String linkName = filename;
        if (linkName.startsWith("/")) linkName = linkName.substring(1);
        html += "<li><a href='/viewcsv?file=" + linkName + "'>" + filename + "</a></li>";
        count++;
      }
      file = root.openNextFile();
    }

    html += "</ul>";
    html += "<p><strong>Total CSV files:</strong> " + String(count) + "</p>";
    html += "</body></html>";

    request->send(200, "text/html", html);
  });

  // Route to view specific CSV file as HTML table
  server.on("/viewcsv", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->hasParam("file")) {
      request->send(400, "text/plain", "Missing file parameter");
      return;
    }

    String filename = request->getParam("file")->value();

    // Normalize filename: ensure it starts with "/" and prevent directory traversal
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    if (filename.indexOf("..") >= 0) {
      request->send(400, "text/plain", "Invalid file path");
      return;
    }

    if (!SD.exists(filename)) {
      request->send(404, "text/plain", "File not found: " + filename);
      return;
    }

    File file = SD.open(filename);
    if (!file || file.isDirectory()) {
      request->send(500, "text/plain", "Failed to open file: " + filename);
      return;
    }

    String html = "<html><head><title>" + filename + "</title></head><body>";
    html += "<h2>Viewing: " + filename + "</h2>";
    html += "<a href='/listcsv'>Back to file list</a><br><br>";
    html += "<table border='1' cellpadding='5' cellspacing='0'>";

    while (file.available()) {
      String line = file.readStringUntil('\n');
      html += "<tr>";
      int start = 0;
      int commaIndex;
      while ((commaIndex = line.indexOf(',', start)) != -1) {
        html += "<td>" + line.substring(start, commaIndex) + "</td>";
        start = commaIndex + 1;
      }
      html += "<td>" + line.substring(start) + "</td>";  // last column
      html += "</tr>";
    }

    html += "</table></body></html>";
    file.close();

    request->send(200, "text/html", html);
  });
  server.begin();
}
// logSDCard();
// readSDcard();

void loop() {
  if (millis() - pmillis > 2000) {
    getReadings();
    logSDCard();
    pmillis = millis();
  }
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
      // Serial.print("Temperature from sensor ");
      // Serial.print(i + 1);
      // Serial.print(" (pin ");
      // Serial.print(sensorPins[i]);
      // Serial.print("): ");
      // Serial.print(Temp[i]);
      // Serial.println(" °C");
    }
  }
  // Serial.println();
}

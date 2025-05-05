#include "Temp.h"
#include <RTClib.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

RTC_DS3231 rtc;
AsyncWebServer server(80);

// Wi-Fi and server settings
const char *ssid = "taha";
const char *password = "123456789";
const char *ap_ssid = "RES_LOGGER";
const char *ap_password = "123456789";
IPAddress ap_ip(192, 168, 4, 1);
IPAddress ap_netmask(255, 255, 255, 0);
const char *ServerName = "RES";
#define SD_CARD_SELECT_PIN 5

// Global variables
String timestamp;
unsigned long pmillis = 0;
String Version = "2.0";

typedef struct
{
  String filename;
  String ftype;
  String fsize;
} fileinfo;

fileinfo Filenames[200];
int numfiles = 0;
String webpage;
const int LED = 32;
void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  Serial.begin(115200);
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    digitalWrite(LED, HIGH);
    while (1)
      ;
  }

  findSensors();

  DateTime now = rtc.now();
  timestamp = "/" + String(now.day()) + "-" + String(now.month()) + "-" + String(now.year()) + "_" + String(now.hour()) + "-" + String(now.minute()) + "-" + String(now.second()) + ".csv";
  Serial.println("File to open: " + timestamp);

  // WiFi.begin(ssid, password);
  // if (WiFi.waitForConnectResult() != WL_CONNECTED) {
  //   Serial.println(F("WiFi Failed! Retrying..."));
  //   WiFi.disconnect(false);
  //   delay(500);
  //   WiFi.begin(ssid, password);
  // }
  // Serial.println("IP Address: " + WiFi.localIP().toString());

  // if (!StartMDNSservice(ServerName)) {
  //   Serial.println(F("Error starting mDNS Service"));
  // }

  // Setup WiFi Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ap_ip, ap_ip, ap_netmask);
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("WiFi AP started");
  Serial.println("SSID: " + String(ap_ssid));
  Serial.println("IP Address: " + WiFi.softAPIP().toString());

  if (!StartMDNSservice(ServerName)) {
    Serial.println(F("Error starting mDNS Service"));
  }

  if (!SD.begin(SD_CARD_SELECT_PIN)) {
    Serial.println(F("Error initializing SD card"));
  } else {
    Int_Card();
  }

  // Root redirects to /dir
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Dir(request);
    request->send(200, "text/html", webpage);
  });

  // Directory handler
  server.on("/dir", HTTP_GET, [](AsyncWebServerRequest *request) {
    Dir(request);
    request->send(200, "text/html", webpage);
  });

  // Download handler
  server.on("/downloadhandler", HTTP_GET, [](AsyncWebServerRequest *request) {
    String url = request->url();
    int startIndex = url.indexOf('/', 1) + 1;
    String filename = url.substring(startIndex);
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    Serial.println("Attempting to download file: " + filename);
    File file = SD.open(filename.c_str(), "r");
    if (file) {
      Serial.println("File found, size: " + String(file.size()) + " bytes");
      AsyncWebServerResponse *response = request->beginResponse("application/octet-stream", file.size(), [file](uint8_t *buffer, size_t maxLen, size_t total) mutable -> size_t {
        size_t read = file.read(buffer, maxLen);
        if (read == 0)
          file.close();
        return read;
      });
      response->addHeader("Content-Disposition", "attachment; filename=" + filename.substring(1));
      request->send(response);
    } else {
      Serial.println("File not found: " + filename);
      request->send(404, "text/plain", "File not found: " + filename);
    }
  });

  // Delete handler
  server.on("/deletehandler", HTTP_GET, [](AsyncWebServerRequest *request) {
    String url = request->url();
    int startIndex = url.indexOf('/', 1) + 1;
    String filename = url.substring(startIndex);
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    Serial.println("Attempting to delete file: " + filename);
    if (SD.exists(filename.c_str())) {
      if (SD.remove(filename.c_str())) {
        Serial.println("File deleted: " + filename);
        request->redirect("/dir");
      } else {
        Serial.println("Failed to delete file: " + filename);
        request->send(500, "text/plain", "Failed to delete file: " + filename);
      }
    } else {
      Serial.println("File not found for deletion: " + filename);
      request->send(404, "text/plain", "File not found: " + filename);
    }
  });

  // Not found handler
  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.println("Page not found: " + request->url());
    Page_Not_Found();
    request->send(200, "text/html", webpage);
  });

  server.begin();
  Serial.println(F("System started successfully"));
  Directory();
  digitalWrite(LED, LOW);
}

void loop() {

  digitalWrite(LED, LOW);

  if (millis() - pmillis >= 60000) {
    digitalWrite(LED, HIGH);
    getReadings();
    logSDCard();
    pmillis = millis();
  }
}

void Dir(AsyncWebServerRequest *request) {
  Directory();
  webpage = HTML_Header();
  webpage += F("<h3>File Directory</h3><br>");
  if (numfiles > 0) {
    webpage += F("<table class='center'>");
    webpage += F("<tr><th>Type</th><th>File Name</th><th>File Size</th><th>Action</th></tr>");
    for (int i = 0; i < numfiles; i++) {
      String displayName = Filenames[i].filename.startsWith("/") ? Filenames[i].filename.substring(1) : Filenames[i].filename;
      webpage += "<tr><td>" + Filenames[i].ftype + "</td><td><a href='/downloadhandler/" + displayName + "'>" + displayName + "</a></td><td>" + Filenames[i].fsize + "</td><td><a href='/deletehandler/" + displayName + "' onclick='return promptForPassword(\"Enter password to delete " + displayName + ":\")'>Delete</a></td></tr>";
    }
    webpage += F("</table>");
  } else {
    webpage += F("<h2>No Files Found</h2>");
  }
  webpage += F("<script>");
  webpage += F("function promptForPassword(message) {");
  webpage += F("  var password = prompt(message);");
  webpage += F("  if (password === null) return false;");  // Cancel clicked
  webpage += F("  if (password === '12345') return confirm('Are you sure you want to delete this file?');");
  webpage += F("  alert('Incorrect password!');");
  webpage += F("  return false;");
  webpage += F("}");
  webpage += F("</script>");
  webpage += HTML_Footer();
}

void Directory() {
  numfiles = 0;
  File root = SD.open("/");
  if (root) {
    root.rewindDirectory();
    File file = root.openNextFile();
    while (file && numfiles < 200) {
      digitalWrite(LED, HIGH);
      String fname = file.name();
      if (!fname.startsWith("/")) {
        fname = "/" + fname;
      }
      Filenames[numfiles].filename = fname;
      Filenames[numfiles].ftype = (file.isDirectory() ? "Dir" : "File");
      Filenames[numfiles].fsize = ConvBinUnits(file.size(), 1);
      file = root.openNextFile();
      numfiles++;
    }
    root.close();
  }
  Serial.println("Found " + String(numfiles) + " files on SD card");
}

void Page_Not_Found() {
  webpage = HTML_Header();
  webpage += F("<div class='notfound'>");
  webpage += F("<h1>Error 404 - Page Not Found</h1>");
  webpage += F("<p>The page you were looking for was not found.</p>");
  webpage += F("<p><a href='/'>Return to Files</a></p>");
  webpage += F("</div>");
  webpage += HTML_Footer();
}

String ConvBinUnits(int bytes, int resolution) {
  if (bytes < 1024)
    return String(bytes) + " B";
  if (bytes < 1024 * 1024)
    return String((bytes / 1024.0), resolution) + " KB";
  return String((bytes / 1024.0 / 1024.0), resolution) + " MB";
}

bool StartMDNSservice(const char *Name) {
  esp_err_t err = mdns_init();
  if (err) {
    Serial.printf("MDNS Init failed: %d\n", err);
    return false;
  }
  mdns_hostname_set(Name);
  return true;
}

String HTML_Header() {
  String page;
  page += F("<!DOCTYPE html><html lang='en'><head>");
  page += F("<title>Rex Engineering Solutions</title>");
  page += F("<meta charset='UTF-8'>");
  page += F("<style>");
  page += F("body {width:75em;margin:auto;font-family:Arial,Helvetica,sans-serif;font-size:16px;color:#f5f5f5;background-color:#1a1a1a;text-align:center;}");
  page += F("footer {padding:0.5em;background-color:#800000;color:#f5f5f5;font-size:1.1em;}");
  page += F("table {font-family:arial,sans-serif;border-collapse:collapse;width:70%;margin:auto;}");
  page += F("td, th {border:1px solid #800000;text-align:left;padding:8px;color:#f5f5f5;}");
  page += F("tr:nth-child(even) {background-color:#2a2a2a;}");
  page += F("h1, h2, h3 {color:#800000;}");
  page += F("a {color:#ffcccc;text-decoration:none;}");
  page += F("a:hover {color:#ffffff;}");
  page += F(".topnav {overflow:hidden;background-color:#800000;}");
  page += F(".topnav a {float:left;color:#f5f5f5;padding:0.6em 1em;font-size:1.2em;}");
  page += F(".topnav a:hover {background-color:#600000;}");
  page += F(".notfound {padding:1em;font-size:1.5em;}");
  page += F("</style></head><body>");
  page += F("<div class='topnav'>");
  page += F("<a href='/'>Download Files</a>");
  page += F("</div>");
  page += F("<h1>Rex Engineering Solutions</h1>");
  return page;
}

String HTML_Footer() {
  String page;
  page += F("<footer>");
  page += F("<p>Temperature Logging System</p>");
  page += F("<p>Contact Info: +92-330-8530186</p>");
  page += F("<p>© Rex Engineering Solutions 2025 | Version ");
  page += Version;
  page += F("</p>");
  page += F("</footer></body></html>");
  return page;
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to open file for writing"));
    return;
  }
  if (file.println(message)) {
    Serial.println(F("File written"));
  } else {
    Serial.println(F("Write failed"));
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println(F("Failed to open file for appending"));
    return;
  }
  if (!file.println(message)) {
    Serial.println(F("Append failed"));
  }
  file.close();
}

void Int_Card() {
  File myFile = SD.open(timestamp.c_str(), FILE_WRITE);
  if (myFile) {
    String dataMessage = F("Timestamp,Temp-1,Temp-2,Temp-3,Temp-4");
    appendFile(SD, timestamp.c_str(), dataMessage.c_str());
    Serial.println(F("File Append Init"));
    myFile.close();
  }
}

void logSDCard() {
  DateTime now = rtc.now();
  char tstamp[20];
  snprintf(tstamp, sizeof(tstamp), "%02d-%02d-%04d_%02d-%02d-%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());
  char dataMessage[100];
  snprintf(dataMessage, sizeof(dataMessage), "%s,%.2f,%.2f,%.2f,%.2f", tstamp, Temp[0], Temp[1], Temp[2], Temp[3]);
  Serial.println(dataMessage);
  appendFile(SD, timestamp.c_str(), dataMessage);
}
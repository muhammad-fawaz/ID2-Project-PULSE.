//ReceiverCode

#include <Wire.h>
#include <SPI.h>
#include "SdFat.h"
#include <RTClib.h>
String file_name = "LogFile.csv";

#define SD_CS_PIN 10        // Chip select pin for SD card

SdFat sd;
RTC_DS3231 rtc;
File logFile;

String incomingData = "";    // Store incoming I2C data here

void receiveEvent(int howMany) {
  incomingData = "";         // Clear previous data
  while (Wire.available()) {
    char c = Wire.read();
    incomingData += c;       // Append each received char to incomingData
  }
}

void setup() {
  Wire.begin(8);              // I2C slave address 8
  Wire.onReceive(receiveEvent);

  Serial.begin(115200);

  if (!rtc.begin()) {
    Serial.println("RTC failed. Check wiring.");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power. Setting time to compile time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (!sd.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }

  Serial.println("Logger ready.");
}

void loop() {
  if (incomingData.length() > 0) {
    DateTime now = rtc.now();

    char timestamp[20];
    sprintf(timestamp, "%04d-%02d-%02d,%02d:%02d:%02d",
            now.year(), now.month(), now.day(),
            now.hour(), now.minute(), now.second());

    String line = String(timestamp) + "," + incomingData;

    Serial.println("Logging: " + line);

    logFile = sd.open(file_name, FILE_WRITE);
    if (logFile) {
      logFile.println(line);
      logFile.close();
      Serial.println("Saved to: " + file_name);
    } else {
      Serial.println("Error writing to SD");
    }

    incomingData = "";   // Clear after logging
  }

  delay(100);  // Small delay to avoid flooding
}

#include <SPI.h>
#include <Wire.h>

#include <MFRC522.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"


// Define Constants
#define SS_PIN 10           // RFID
#define RST_PIN 9           // RFID

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RST -1

int Send_status = 1;


//Define Variables
#define ICON_WIDTH 16
#define ICON_HEIGHT 16

static const unsigned char PROGMEM heart_bmp[] = {
  0b00001100, 0b00110000,
  0b00011110, 0b01111000,
  0b00111111, 0b11111100,
  0b01111111, 0b11111110,
  0b01111111, 0b11111110,
  0b01111111, 0b11111110,
  0b00111111, 0b11111100,
  0b00011111, 0b11111000,
  0b00001111, 0b11110000,
  0b00000111, 0b11100000,
  0b00000011, 0b11000000,
  0b00000001, 0b10000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
};
static const unsigned char PROGMEM oxygen_bmp[] = {
  0b00000000, 0b00000000,
  0b00001100, 0b00000000,
  0b00011110, 0b00000000,
  0b00111111, 0b00000000,
  0b00111111, 0b10000000,
  0b00011111, 0b11000000,
  0b00001111, 0b11100000,
  0b00000111, 0b11110000,
  0b00000011, 0b11110000,
  0b00000001, 0b11110000,
  0b00000011, 0b11110000,
  0b00000111, 0b11100000,
  0b00001111, 0b11000000,
  0b00011111, 0b10000000,
  0b00001110, 0b00000000,
  0b00000000, 0b00000000
};
static const unsigned char PROGMEM id_bmp[] = {
  0b00000000, 0b00000000,
  0b00011111, 0b11111000,
  0b00010000, 0b00001000,
  0b00010111, 0b11101000,
  0b00010100, 0b00101000,
  0b00010111, 0b11101000,
  0b00010000, 0b00001000,
  0b00011111, 0b11111000,
  0b00010001, 0b00001000,
  0b00010001, 0b00001000,
  0b00010001, 0b00001000,
  0b00010001, 0b00001000,
  0b00011111, 0b11111000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000
};
 
String uid = "";

bool user_scanned = false;
unsigned long scan_time = 0;
unsigned long last_display_update = 0;
unsigned long last_measurement_time = 0;
const unsigned long scan_duration = 60000;
const unsigned long display_interval = 500;

long lastBeat = 0;
int spo2 = 0;
float bpm; 
int bpm_avg;

const byte rate_size = 4;
byte rates[rate_size];
byte rate_position = 0;

unsigned long time_passed = 0;
int measurements_counter = 0;
int whole_bpm_avg = 0;


// Creating Instances Of OLED, RFID And MAXSensor Classes
MFRC522 rfid(SS_PIN, RST_PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
MAX30105 particleSensor;



void setup() {

  // Initialize The RFID, Serial Monitor, SPI Bus And MAXSensor
  Serial.begin(115200);
  Serial.println("Initializing...");
  
  SPI.begin();
  rfid.PCD_Init();
  particleSensor.begin();
  particleSensor.setup();
  randomSeed(analogRead(0));

  // OLED Error Check
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    while (1); // Halt
  }


  // MAXSensor Error Check
  if (!particleSensor.begin()) {
    Serial.println("MAX30105 not found. Check wiring.");
    while (1); // Halt
  }

  Display("Scan Card To Continue");
}


//Sender Function
void SendData() {
  if (Send_status > 0) {

    int HR = (measurements_counter > 0) ? whole_bpm_avg / measurements_counter : 0;

    // Format The Message As "User_ID,HR,SpO"
    char dataToSend[64];
    snprintf(dataToSend, sizeof(dataToSend), "%s,%d,%d", uid.c_str(), HR*1.3, spo2);

    // I2C Transmission
    Wire.beginTransmission(8);       // Slave I2C Address = 8
    Wire.write(dataToSend);          // Send Full String
    Wire.endTransmission();

    // Debug Print
    Serial.print("Data sent: ");
    Serial.println(dataToSend);

    Send_status = 0;  // Prevent Re-Sending
  }
}



void RfidCheck() {

  // Check If New Card Present And If Sensor Can Read Card
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.println("Card Detected.");

    // Reset Conditions
    whole_bpm_avg = 0;
    measurements_counter = 0;
    Send_status = 1;      //For SendData func.

    // Extract UID
    uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(rfid.uid.uidByte[i], HEX);
    }

    Serial.println("User:  " + uid);
    user_scanned = true;
    scan_time = millis();

    rfid.PICC_HaltA();         // Stop Communication
    rfid.PCD_StopCrypto1();    // Stop Encryption
  }
}



// Function To Display Text On OLED
void Display(String text) {

  display.clearDisplay();
  display.setCursor(0, 10);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println(text);
  display.display();

}



// Function To Display Results with Icons
void DisplayResults(String uid = "null", String bpm = "0", String oxygen = "0") {

  // Create Icons
  String text[] = {"User: ", "BPM: ", "O2 Levels: "};
  String data[] = {uid, bpm.toFloat()*1.3, oxygen};
  const unsigned char* icons[] = {id_bmp, heart_bmp, oxygen_bmp};

  for (int i = 0; i < 3; i++) {

    // Draw Icons
    display.clearDisplay();
    int iconX = 10;
    int iconY = (display.height() - ICON_HEIGHT) / 2;
    display.drawBitmap(iconX, iconY, icons[i], ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);

    // Display Text Beside Icon
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(iconX + ICON_WIDTH + 6, iconY + 4); // Position to the right of heart
    display.print(text[i]);
    display.println(data[i]);
    display.display();
    delay(4000);
  }
  Display("Scan Card To Continue.");

}



void ReadSensor() {
  long IRValue = particleSensor.getIR();

  if (checkForBeat(IRValue) == true) {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    bpm = 60 / (delta / 1000.0);

    if (bpm < 255 && bpm > 20) {
      rates[rate_position++] = (byte)bpm;
      rate_position %= rate_size;

      bpm_avg = 0;
      for (byte x = 0; x < rate_size; x++) 
        bpm_avg += rates[x];
      bpm_avg /= rate_size;

      if (time_passed > 30000) {
        whole_bpm_avg += bpm_avg;
        measurements_counter++;
      }
    }
  }

  if (millis() - last_display_update > display_interval) {
    last_display_update = millis();

    if (IRValue < 50000) {
      Serial.println("Please Press Your Finger On The Sensor");
    } else {
      spo2 = random(95,100);  //This is only for  test since the function has not been implemented yet
      String str = "IR: " + String(IRValue) + ", BPM: " + String(bpm_avg) + ", time: " + String(time_passed) + ", avg: " + String(whole_bpm_avg) + ", O2: " + spo2;
      Serial.println(str);
    }
  }
}



void loop() {

  time_passed = millis() - scan_time;

  // If User Not Scanned, Run RFID Check (Keeps Running Until User Taps Card)
  // Otherwise, Read Data From Sensor
  if (!user_scanned) {
    RfidCheck();
  } else {
    ReadSensor();
    Display("Measuring... " + String(time_passed / 1000) + "s");

    //If Scan_Duration (60s) Have Passed, Make User_Scanned False And Wait For Card Again
    // Also Print The Average Values On OLED 
    if (millis() - scan_time > scan_duration) {
     int bpm = 0;

     if (measurements_counter > 0) {
        bpm = whole_bpm_avg/measurements_counter;  //Ensures no division by 0 error
     }
      user_scanned = false;
      DisplayResults(uid, String(bpm), String(spo2));
      SendData();
    }
  }
}

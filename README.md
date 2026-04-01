## Overview

This project was developed during my first year at my university. It serves as a proof-of-concept for a modular health-monitoring station that identifies users via RFID, records real-time heart rate and $SpO_2$ data, and logs the results to a CSV file with an accurate timestamp. Below are some notes I used during presentation.

## Features

- User Authentication: Secure identification using MFRC522 RFID tags.
- Biometric Sensing: Real-time heart rate and $SpO_2$ simulation using the MAX30105 particle sensor.
- Data Logging: Reliable data storage to SD cards using the SdFat library, with timestamping via an external RTC (DS3231).
- Communication: Inter-device communication between the sensor-unit and the logger-unit via I2C protocol.

## Components

- Microcontrollers: 2x Arduino (or compatible boards).
- MAX30105 (Heart Rate & $SpO_2$)
- MFRC522 (RFID)
- DS3231 (RTC)
- SSD1306 (OLED Display)
- SD Card Module

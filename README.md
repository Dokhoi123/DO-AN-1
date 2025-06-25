# ESP32 Fingerprint & Password Door Lock System

This project implements a secure door access control system using an ESP32 microcontroller. It features a dual authentication mechanism, requiring either a password or a registered fingerprint to grant access. The system is managed through a 4x4 keypad and provides feedback to the user via a 16x2 I2C LCD display.

## Features

* **Dual Authentication:** Unlock via a 4-digit password or a stored fingerprint.
* **Persistent Storage:** The password is saved in the ESP32's EEPROM, so it is retained even after a power loss.
* **User Management:** An administrative "Edit Mode" allows for:
    * Changing the access password.
    * Enrolling new fingerprints with a specific ID (0-99).
    * Deleting existing fingerprints by their ID.
* **Interactive Display:** A 16x2 LCD screen guides the user through all operations, such as entering a password, enrolling fingerprints, and shows access status (granted or denied).
* **Security Alarm:** A buzzer will sound an alarm after three consecutive failed access attempts (either by password or fingerprint).
* **Multitasking:** Utilizes FreeRTOS to handle multiple tasks concurrently (keypad input, fingerprint scanning, and device state management).

![image](https://github.com/user-attachments/assets/7096d951-4a3f-45a4-98c8-1fe7915fda27)

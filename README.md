ESP32 Smart Door Lock System with Fingerprint and Password
This is a DIY project for a secure and versatile smart door lock system using the ESP32 microcontroller. The system offers two authentication methods to open the door: a fingerprint sensor and a password via a matrix keypad. This project is built on the Arduino platform with FreeRTOS, enabling smooth and efficient handling of multiple concurrent tasks.

Key Features
Multiple Unlocking Methods:

Password: Use a 4x4 matrix keypad to enter a 4-digit password.

Fingerprint: Use an optical fingerprint sensor for fast and secure recognition.

Intuitive User Interface:

A 16x2 LCD screen displays instructions, status, and notifications to the user.

A buzzer provides audible feedback for key presses, successful/failed authentications, and alarms.

Administrator Mode:

An admin menu protected by the current password.

Allows the user to change the password, enroll new fingerprints, and delete saved fingerprints.

Security and Durability:

Password Storage: The password is saved to the ESP32's EEPROM, so it is not lost when power is disconnected. The default password is 2018.

Alarm System: The buzzer will sound if the password or fingerprint is entered incorrectly 3 consecutive times.

Electric Lock: Controls a relay to open/close an electric door latch.

Multitasking:

Utilizes the FreeRTOS real-time operating system to manage concurrent tasks such as fingerprint scanning, password entry, and door control, ensuring the system is always responsive.
![image](https://github.com/user-attachments/assets/7096d951-4a3f-45a4-98c8-1fe7915fda27)

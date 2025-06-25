
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_task_wdt.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <EEPROM.h> 

#define EEPROM_SIZE 64 // chỉ cần nhỏ thôi
#define PASS_ADDR 0



#define BUTTON  15
#define buzzer 2
#define relay 4

HardwareSerial mySerial(0);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'D','C','B','A'},
  {'#','9','6','3'},
  {'0','8','5','2'},
  {'*','7','4','1'}
};

byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

char STR[4] = {'2', '0', '1', '8'};
char str[4] = {' ', ' ', ' ', ' '};
int i, count = 0;
int BUTTONState = 0;
int wrong = 0;
xSemaphoreHandle Sem_Hanle;

void nhappass(void*);
void open_door(void*);
void doipass(void*);
void baodong(void*);
void check_fingers(void*);
void enrollFingerprint();
int getFingerprintID();
bool deleteFingerprint(int id);
volatile bool isEnrolling = false;

void readPasswordFromEEPROM() {
  for (int i = 0; i < 4; i++) {
    char c = EEPROM.read(PASS_ADDR + i);
    if (c >= 32 && c <= 126) {
      STR[i] = c;
    } else {
      STR[i] = '0'; // nếu EEPROM chưa có gì, mặc định là '0'
    }
  }
}

void savePasswordToEEPROM(char newPass[4]) {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(PASS_ADDR + i, newPass[i]);
  }
  EEPROM.commit();
}


bool isEditCommand(char str[4]) {
  return str[0] == '*' && str[1] == '*' && str[2] == '*' && str[3] == '*';
}

void setup() {
  Sem_Hanle = xSemaphoreCreateCounting(1, 0);
  Serial.begin(9600);
  EEPROM.begin(EEPROM_SIZE);
  
  readPasswordFromEEPROM();
  if (STR[0] == '0' && STR[1] == '0' && STR[2] == '0' && STR[3] == '0') {
  STR[0] = '2'; STR[1] = '0'; STR[2] = '1'; STR[3] = '8';
  savePasswordToEEPROM(STR);
}
  mySerial.begin(9600, SERIAL_8N1);
  lcd.init();
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.print("Enter password: ");
  
  pinMode(relay, OUTPUT);
  pinMode(BUTTON ,INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);

  Serial.println("Initializing fingerprint sensor...");
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor detected!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (true);
  }

  xTaskCreate(nhappass, "nhap mat khau", 4096, NULL, 2, NULL);
  xTaskCreate(open_door, "open_door", 4096, NULL, 0, NULL);
  
  xTaskCreate(baodong, "baodong", 4096, NULL, 0, NULL);
  xTaskCreate(check_fingers, "check_fingers", 4096, NULL, 1, NULL);
}

void enterEditMode() {
  lcd.clear();
  lcd.print("Nhap MK hien tai");
  char tempPass[4] = {' ', ' ', ' ', ' '};
  char key;
  int j = 0;

  while (j < 4) {
    key = customKeypad.getKey();
    if (key) {
      tempPass[j] = key;
      lcd.setCursor(6 + j, 1);
      lcd.print("*");
      j++;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  // So sánh với mật khẩu hiện tại
  if (memcmp(tempPass, STR, 4) != 0) {
    lcd.clear();
    lcd.print("Incorrect");
    delay(2000);
    return;  // Thoát khỏi hàm nếu sai mật khẩu
  }

  // Đúng mật khẩu thì tiếp tục
  lcd.clear();
  lcd.print("A: Doi MK");
  lcd.setCursor(0, 1);
  lcd.print("B: Them C: Xoa");

  key = NO_KEY;
  while (key == NO_KEY) {
    key = customKeypad.getKey();
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  if (key == 'A') {
    lcd.clear();
    lcd.print("New Password:");
    int i = 0;
    while (i < 4) {
      key = customKeypad.getKey();
      if (key) {
        STR[i] = key;
        lcd.setCursor(6 + i, 1);
        lcd.print("*");
        i++;
        vTaskDelay(pdMS_TO_TICKS(300));
      }
    }
    savePasswordToEEPROM(STR);
    lcd.clear();
    lcd.print("MK moi da luu");
    delay(2000);
  }
  else if (key == 'B') {
    enrollFingerprint();
  }
  else if (key == 'C') {
    lcd.clear();
    lcd.print("Nhap ID xoa:");
    char idStr[3] = {0};
    int idIndex = 0;
    while (idIndex < 2) {
      key = customKeypad.getKey();
      if (key && isdigit(key)) {
        idStr[idIndex] = key;
        lcd.setCursor(6 + idIndex, 1);
        lcd.print(key);
        idIndex++;
      }
    }
    int idToDelete = atoi(idStr);
    lcd.clear();
    lcd.print("Xoa ID: ");
    lcd.print(idToDelete);
    delay(2000);

    if (deleteFingerprint(idToDelete)) {
      lcd.clear();
      lcd.print("Xoa thanh cong");
    } else {
      lcd.clear();
      lcd.print("Xoa that bai");
    }
    delay(3000);
  }
}


void nhapmatkhau(void) {
  char key = customKeypad.getKey();
  if (key) {
    if (i < 4) {
      str[i] = key;
      lcd.setCursor(6 + i, 1);
      lcd.print(str[i]);
      delay(500);
      lcd.setCursor(6 + i, 1);
      lcd.print("*");
      i++;
      if (i == 4) count = 1;
    }
  }
}

void checkmatkhau(void) {
  if (count == 1) {
    if (isEditCommand(str)) {
      lcd.clear();
      lcd.print("Vao che do sua");
      delay(2000);
      i = 0;
      count = 0;
      // gọi trực tiếp chức năng đổi mật khẩu
      enterEditMode(); // ==> sẽ tạo thêm hàm này bên dưới
      lcd.clear();
      lcd.print("Enter password");
    }
    else if (memcmp(str, STR, 4) == 0) {
      lcd.clear();
      lcd.print("    Correct!");
      wrong = 0;
      delay(3000);
      lcd.clear();
      lcd.print("Enter Password:");
      xSemaphoreGive(Sem_Hanle);
    } else {
      lcd.clear();
      lcd.print("   Incorrect!");
      delay(2000);
      wrong++;
      lcd.clear();
      lcd.print("   Try Again!");
      delay(2000);
      lcd.clear();
      lcd.print("Enter Password:");
    }
    i = 0;
    count = 0;
  }
}


void open_door(void*) {
  while (1) {
    delay(1000);
    if (xSemaphoreTake(Sem_Hanle, portMAX_DELAY) == pdTRUE) {
      Serial.println("Task1");
      digitalWrite(relay, HIGH);
      delay(3000);
      digitalWrite(relay, LOW);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

void nhappass(void*) {
  while (1) {
    nhapmatkhau();
    checkmatkhau();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void baodong(void*) {
  while (1) {
    if (wrong == 3) {
      //tone(buzzer, 5000);
      digitalWrite(2,HIGH);
      delay(3000);
      wrong = 0;
      digitalWrite(2,LOW);
      //delay(3000);
      //noTone(buzzer);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void check_fingers(void*) {
  

  while (1) {
     if (isEnrolling) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
  }

    int p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      int id = getFingerprintID();
      if (id != -1) {
        Serial.print("Detected fingerprint ID: ");
        Serial.println(id);
        lcd.clear();
        lcd.print("Correct");
        xSemaphoreGive(Sem_Hanle);
        wrong = 0;
        delay(4000);
        lcd.clear();
        lcd.print("Enter password");
      } else {
        lcd.clear();
        lcd.print("False");
        wrong++;
        delay(4000);
        lcd.clear();
        lcd.print("Enter password");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

int getFingerprintID() {
  if (finger.getImage() != FINGERPRINT_OK) return -1;
  if (finger.image2Tz() != FINGERPRINT_OK) return -1;
  if (finger.fingerFastSearch() != FINGERPRINT_OK) return -1;
  return finger.fingerID;
}

bool getFingerprintEnroll(int id) {
  int p = -1;
  Serial.println("Place your finger on the sensor...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) delay(100);
    else if (p == FINGERPRINT_IMAGEFAIL) return false;
  }

  if (finger.image2Tz(1) != FINGERPRINT_OK) return false;
  lcd.clear(); lcd.print("nhan hinh anh"); delay(1000);

  lcd.clear(); lcd.print("tha ra");
  Serial.println("Remove your finger...");
  delay(3000);
  while (finger.getImage() != FINGERPRINT_NOFINGER) {}

  lcd.clear(); lcd.print("Dat lai van tay");
  Serial.println("Place the same finger again...");
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) continue;
    else if (p == FINGERPRINT_IMAGEFAIL) return false;
  }

  if (finger.image2Tz(2) != FINGERPRINT_OK) return false;
  if (finger.createModel() != FINGERPRINT_OK) return false;
  if (finger.storeModel(id) != FINGERPRINT_OK) return false;

  Serial.println("Fingerprint stored!");
  return true;
}

void enrollFingerprint() {
  isEnrolling = true;

  lcd.clear(); lcd.print("NHAP ID:");
  Serial.println("Ready to enroll a fingerprint!");
  char idStr[3] = {0};
  int idIndex = 0;
  char key;
  while (idIndex < 2) {
    key = customKeypad.getKey();
    if (key && isdigit(key)) {
      idStr[idIndex] = key;
      lcd.setCursor(6 + idIndex, 1);
      lcd.print(key);
      idIndex++;
    }
  }
  delay(1000);
  int id = atoi(idStr);
  lcd.clear(); lcd.print("ID: "); lcd.print(id);
  delay(1000);
  lcd.clear(); lcd.print("Them van tay");

  if (getFingerprintEnroll(id)) {
    lcd.clear(); lcd.print("Enroll done");
    delay(3000);
  } else {
    lcd.clear(); lcd.print("Enroll fail");
    delay(3000);
  }
  lcd.clear(); lcd.print("Enter password");
  isEnrolling = false;
}

bool deleteFingerprint(int id) {
  int p = finger.deleteModel(id);
  return (p == FINGERPRINT_OK);
}

void loop() {}

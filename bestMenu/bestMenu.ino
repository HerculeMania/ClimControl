#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Wire.h>
#include "system.h"
void prepareMessage();
void requestEvent();
byte txData[MESSAGE_DASH_RX_LENGTH];
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

const int leftButtonPin = 8;
const int rightButtonPin = 9;

struct MenuItem {
  char* name;
  char** options;
  uint8_t optionCount;
  uint8_t selectedIndex;
  bool isTemperature;
  uint8_t temperatureValue;
  bool showFahrenheit;
};

char* airCondOptions[] = { (char*)"OFF", (char*)"ON Driver", (char*)"ON Passe." };
char* temperOptions[] = { (char*)"Celsius", (char*)"Fahrenheit" };
char* fanSpeedOptions[] = { (char*)"Low", (char*)"High", (char*)"Auto" };
char* airRecOptions[] = { (char*)"Fresh", (char*)"Recircu." };

MenuItem menu[] = {
  { (char*)"Air Cond", airCondOptions, 3, 0, false, 0, false },
  { (char*)"Temper  ", temperOptions, 2, 0, true, 20, false },
  { (char*)"Fan Speed", fanSpeedOptions, 3, 0, false, 0, false },
  { (char*)"Air Rec ", airRecOptions, 2, 0, false, 0, false }
};

const int menuLength = sizeof(menu) / sizeof(menu[0]);
int currentItem = 0;
bool settingMode = false;
bool browseFullView = true;

unsigned long lastBrowseInteractionTime = 0;
unsigned long lastSettingInteractionTime = 0;
unsigned long arrowBlinkStartTime = 0;
bool arrowShouldBlink = false;

bool blinkVisible = true;
unsigned long blinkStartTime = 0;
const unsigned long blinkInterval = 300;

const unsigned long longPressThreshold = 1500;

bool leftPressed = false;
bool rightPressed = false;
bool leftLongPressDetected = false;
bool rightLongPressDetected = false;
bool longPressBlinking = false;
unsigned long leftLongPressStartTime = 0;
unsigned long rightLongPressStartTime = 0;

const int EEPROM_START = 0;

void setup() {
  lcd.begin(20, 4);
  Serial.begin(9600);
  pinMode(leftButtonPin, INPUT_PULLUP);
  pinMode(rightButtonPin, INPUT_PULLUP);
  showStartupScreen();
  loadSettingsFromEEPROM();
  displayCurrentItem();
  Wire.begin(DASH_ADDRESS);  // Slava adde = DASH_ADDRESS
  Wire.onRequest(requestEvent);
  //systemOn =0;
}

void showStartupScreen() {
  lcd.clear();
  printCentered("Clim Control", 0);
  printCentered("HerculeMania", 2);
  printCentered("+243993083211", 3);
  delay(2000);
  lcd.clear();
  printCentered("herculevahviraki", 1);
  printCentered("@gmail.com", 2);
  delay(2000);
}

void printCentered(const char* text, uint8_t row) {
  int len = strlen(text);
  int padding = (20 - len) / 2;
  lcd.setCursor(padding, row);
  lcd.print(text);
}

void saveSettingsToEEPROM() {
  int addr = EEPROM_START;
  for (int i = 0; i < menuLength; i++) {
    EEPROM.update(addr++, menu[i].selectedIndex);
    EEPROM.update(addr++, menu[i].temperatureValue);
    EEPROM.update(addr++, menu[i].showFahrenheit ? 1 : 0);
  }
}

void loadSettingsFromEEPROM() {
  int addr = EEPROM_START;
  for (int i = 0; i < menuLength; i++) {
    menu[i].selectedIndex = EEPROM.read(addr++);
    uint8_t temp = EEPROM.read(addr++);
    menu[i].temperatureValue = (i == 1 && temp > 100) ? 20 : temp;
    menu[i].showFahrenheit = EEPROM.read(addr++) == 1;
    // Force Air Cond to OFF on startup
    if (i == 0) {
      menu[i].selectedIndex = 0;
    }
  }
}

void loop() {
  handleButtons();
  handleBlink();
  prepareMessage();
  if (digitalRead(leftButtonPin) == LOW && digitalRead(rightButtonPin) == LOW) {
    if (settingMode && menu[currentItem].isTemperature) {
      menu[currentItem].showFahrenheit = !menu[currentItem].showFahrenheit;
      //Serial.println(menu[currentItem].showFahrenheit ? "Toggled to Fahrenheit" : "Toggled to Celsius");
      delay(300);
      displayCurrentItem();
    }
  }

  if (!settingMode && !browseFullView && millis() - lastBrowseInteractionTime >= 10000) {
    browseFullView = true;
    arrowBlinkStartTime = millis();
    arrowShouldBlink = true;
    displayCurrentItem();
  }

  if (settingMode && millis() - lastSettingInteractionTime >= 20000) {
    settingMode = false;
    //Serial.println("Exited SET mode due to timeout");
    displayCurrentItem();
  }

  if (arrowShouldBlink && millis() - arrowBlinkStartTime >= 10000) {
    arrowShouldBlink = false;
  }
}

void handleButtons() {
  int leftState = digitalRead(leftButtonPin);
  if (leftState == LOW && !leftPressed) {
    leftPressed = true;
    leftLongPressStartTime = millis();
    leftLongPressDetected = false;
    longPressBlinking = false;
  }

  if (leftPressed && !leftLongPressDetected && millis() - leftLongPressStartTime >= longPressThreshold) {
    leftLongPressDetected = true;
    longPressBlinking = true;
    //Serial.println("Left long press validated");
  }

  if (leftState == HIGH && leftPressed) {
    leftPressed = false;
    longPressBlinking = false;
    if (leftLongPressDetected) {
      leftLongPressDetected = false;
      if (!settingMode) {
        settingMode = true;
        lastSettingInteractionTime = millis();
        //Serial.println("Entering SET mode");
      } else {
        settingMode = false;
        browseFullView = true;
        saveSettingsToEEPROM();
        //Serial.println("Settings saved");
      }
      displayCurrentItem();
    } else {
      //Serial.println("Short press LEFT");
      handleShortPress(true);
    }
  }

  int rightState = digitalRead(rightButtonPin);
  if (rightState == LOW && !rightPressed) {
    rightPressed = true;
    rightLongPressStartTime = millis();
    rightLongPressDetected = false;
  }

  if (rightPressed && !rightLongPressDetected && millis() - rightLongPressStartTime >= longPressThreshold) {
    rightLongPressDetected = true;
    //Serial.println("Right long press validated");
  }

  if (rightState == HIGH && rightPressed) {
    rightPressed = false;
    if (rightLongPressDetected) {
      rightLongPressDetected = false;
      if (settingMode) {
        loadSettingsFromEEPROM();
        settingMode = false;
        browseFullView = true;
        //Serial.println("Settings discarded");
        displayCurrentItem();
      }
    } else {
      //Serial.println("Short press RIGHT");
      handleShortPress(false);
    }
  }
}

void handleShortPress(bool isLeft) {
  if (settingMode) {
    lastSettingInteractionTime = millis();
    switch (currentItem) {
      case 1:
        if (isLeft && menu[1].temperatureValue < 100) menu[1].temperatureValue++;
        if (!isLeft && menu[1].temperatureValue > 0) menu[1].temperatureValue--;
        break;
      default:
        if (isLeft) menu[currentItem].selectedIndex = (menu[currentItem].selectedIndex + 1) % menu[currentItem].optionCount;
        else menu[currentItem].selectedIndex = (menu[currentItem].selectedIndex + menu[currentItem].optionCount - 1) % menu[currentItem].optionCount;
        break;
    }
    blinkStartTime = millis();
    blinkVisible = true;
    displayCurrentItem();
  } else {
    lastBrowseInteractionTime = millis();
    if (isLeft) {
      browseFullView = !browseFullView;
      arrowBlinkStartTime = millis();
      arrowShouldBlink = true;
    } else {
      currentItem = (currentItem + 1) % menuLength;
      arrowBlinkStartTime = millis();
      arrowShouldBlink = true;
    }
    displayCurrentItem();
  }
}

void handleBlink() {
  if (longPressBlinking && millis() - blinkStartTime >= blinkInterval) {
    blinkVisible = !blinkVisible;
    blinkStartTime = millis();
    displayCurrentItem();
  }
}

void displayCurrentItem() {
  lcd.clear();

  if (settingMode) {
    printCentered(menu[currentItem].name, 0);
    lcd.setCursor(0, 2);

    if (!blinkVisible) {
      lcd.print("                    ");
    } else {
      if (menu[currentItem].isTemperature) {
        uint8_t temp = menu[currentItem].temperatureValue;
        if (menu[currentItem].showFahrenheit) {
          temp = round(temp * 9.0 / 5.0 + 32);
          lcd.print("Temp: ");
          lcd.print(temp);
          lcd.print((char)223);
          lcd.print("F");
        } else {
          lcd.print("Temp: ");
          lcd.print(temp);
          lcd.print((char)223);
          lcd.print("C");
        }
      } else {
        lcd.print("Mode: ");
        lcd.print(menu[currentItem].options[menu[currentItem].selectedIndex]);
      }
    }

    lcd.setCursor(0, 3);
    lcd.print("SET");
    lcd.setCursor(17, 3);
    lcd.print(currentItem + 1);
    lcd.print("/");
    lcd.print(menuLength);

  } else {
    if (browseFullView) {
      for (int i = 0; i < menuLength && i < 4; i++) {
        lcd.setCursor(0, i);
        lcd.print(i == currentItem ? (arrowShouldBlink && millis() - arrowBlinkStartTime < 10000 ? (longPressBlinking && !blinkVisible ? " " : ">") : " ") : " ");

        if (longPressBlinking && i == currentItem && !blinkVisible) {
          lcd.print("                    ");
        } else {
          lcd.print(menu[i].name);
          lcd.print(": ");
          if (menu[i].isTemperature) {
            uint8_t temp = menu[i].temperatureValue;
            if (menu[i].showFahrenheit) {
              temp = round(temp * 9.0 / 5.0 + 32);
              lcd.print(temp);
              lcd.print((char)223);
              lcd.print("F");
            } else {
              lcd.print(temp);
              lcd.print((char)223);
              lcd.print("C");
            }
          } else {
            lcd.print(menu[i].options[menu[i].selectedIndex]);
          }
        }
      }
    } else {
      lcd.setCursor(0, 0);
      lcd.print(">");
      if (longPressBlinking && !blinkVisible) {
        lcd.print("                    ");
      } else {
        lcd.print(menu[currentItem].name);
        lcd.print(": ");
        if (menu[currentItem].isTemperature) {
          uint8_t temp = menu[currentItem].temperatureValue;
          if (menu[currentItem].showFahrenheit) {
            temp = round(temp * 9.0 / 5.0 + 32);
            lcd.print(temp);
            lcd.print((char)223);
            lcd.print("F");
          } else {
            lcd.print(temp);
            lcd.print((char)223);
            lcd.print("C");
          }
        } else {
          lcd.print(menu[currentItem].options[menu[currentItem].selectedIndex]);
        }
      }

      lcd.setCursor(17, 3);
      lcd.print(currentItem + 1);
      lcd.print("/");
      lcd.print(menuLength);
    }
  }
}

void prepareMessage() {
  for (int i = 0; i < MAXSHOWNVALUE; i++) {
    if (i == 1)
      txData[i] = menu[i].temperatureValue;
    else
      txData[i] = menu[i].selectedIndex;
  }
  txData[MAXSHOWNVALUE] = map(analogRead(A0), 0, 1023, 0, MAXSENSOREAD);
  txData[MAXSHOWNVALUE + 1] = map(analogRead(A1), 0, 1023, 0, MAXSENSOREAD);
  txData[MAXSHOWNVALUE + 2] = map(analogRead(A2), 0, 1023, 0, MAXSENSOREAD);
  txData[MAXSHOWNVALUE + 3] = map(analogRead(A3), 0, 1023, 0, MAXSENSOREAD);
  txData[MAXSHOWNVALUE + 4] = map(analogRead(A6), 0, 1023, 0, MAXSENSOREAD);
  txData[MAXSHOWNVALUE + 5] = 0;
}
void requestEvent() {
  Wire.write(txData, MESSAGE_DASH_RX_LENGTH);
}
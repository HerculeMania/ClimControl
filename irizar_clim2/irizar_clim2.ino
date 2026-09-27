#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Wire.h>
#include "system.h"
#define EVENEMENT_BUTTON_LEFT 1
#define EVENEMENT_BUTTON_RIGHT 2

byte txData[MESSAGE_DASH_RX_LENGTH];//+systemOn value
const uint8_t stateValMax[MAXSHOWNVALUE] = { 3, MAXSETUPTEMPVAL, 3, 3 };
uint8_t stateVal[2][MAXSHOWNVALUE];
//#define DEBUG_BUTTON
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 2, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
//buttons
uint8_t b_left = 8, b_right = 9;  //button setup
uint8_t timeL, timeR;
//
uint8_t valTemp = 20;
//draw valus
uint8_t state[3][4] = { " ON", "OFF", "AUT" };
uint8_t setupPos = 0;
uint8_t systemOn = 0;
// two byte union for events
typedef union _U {
  struct {
    uint8_t type;
    uint8_t value;
  };
  uint16_t bytes2;
} Ubytes2;
Ubytes2 event;
//
//events valuse
Ubytes2 ev;
//
uint8_t eventCount;
//end of global
//AC values

//
//button long press and short
uint8_t leftP, rightP;
unsigned long currentMillisLeft, onPressedLeft;
unsigned long currentMillisRight, onPressedRight;
void process_button(uint8_t* timeLeft, uint8_t* timeRight) {
  if (!digitalRead(b_left) && leftP) {
    leftP = 0;
    onPressedLeft = millis() / 1000;
  }
  if (digitalRead(b_left) && !leftP) {
    leftP = 1;
    *timeLeft = currentMillisLeft - onPressedLeft;
    event.value = *timeLeft;
    event.type = EVENEMENT_BUTTON_LEFT;
    eventCount = 1;
  }
  if (!digitalRead(b_left) && !leftP) {
    currentMillisLeft = millis() / 1000;
  }
  if (!digitalRead(b_right) && rightP) {
    rightP = 0;
    onPressedRight = millis() / 1000;
  }
  if (digitalRead(b_right) && !rightP) {
    rightP = 1;
    *timeRight = currentMillisRight - onPressedRight;

    event.value = *timeRight;
    event.type = EVENEMENT_BUTTON_RIGHT;
    eventCount = 1;
  }
  if (!digitalRead(b_right) && !rightP) {
    currentMillisRight = millis() / 1000;
  }
}
// //send message
uint8_t messageBlinkState, messageLastState;
unsigned long messageTimeStart, messageTimeCount;
const uint8_t condensorPin = A1, evaMinPin = A3, EvaMaxPin = 8, clutchPin = 6;
void prepareMessage() {
  for (int i = 0; i < MAXSHOWNVALUE; i++) {
    txData[i] = stateVal[0][i];
  }
  txData[MAXSHOWNVALUE] = map(analogRead(A0), 0, 1023, 0, MAXSENSOREAD);
  txData[MAXSHOWNVALUE+1] = map(analogRead(A1), 0, 1023, 0, MAXSENSOREAD);
  txData[MAXSHOWNVALUE+2] = map(analogRead(A2), 0, 1023, 0, MAXSENSOREAD);
  txData[MAXSHOWNVALUE+3] = map(analogRead(A3), 0, 1023, 0, MAXSENSOREAD);
  txData[MAXSHOWNVALUE+4] = map(analogRead(A6), 0, 1023, 0, MAXSENSOREAD);
  txData[MAXSHOWNVALUE+5] = systemOn;
}


// Write a byte to EEPROM only if it's different (preserves write cycles)
void EEPROMwriteByte(int address, byte value) {
  if (EEPROM.read(address) != value) {
    EEPROM.write(address, value);
  }
}

// Read a byte from EEPROM
byte EEPROMreadByte(int address) {
  return EEPROM.read(address);
}

void EEPROMwriteBytes(int startAddress, byte* data, int length) {
  for (int i = 0; i < length; i++) {
    EEPROM.update(startAddress + i, data[i]);  // safer than write()
  }
}
void EEPROMreadBytes(int startAddress, byte* buffer, int length) {
  for (int i = 0; i < length; i++) {
    buffer[i] = EEPROM.read(startAddress + i);
  }
}

//
// event function
uint8_t eventRead(uint16_t* e) {
  if (eventCount == 0) {
    return 0;
  } else {
    *e = event.bytes2;
    eventCount = 0;
    return 1;
  }
}
unsigned long menuStages;
unsigned long countMenuToReturn;
unsigned long countMenuEventStarted;
uint8_t prepareMenu12, prepareMenu23;
void process_menu() {
  if (menuStages > 0) {
    countMenuToReturn = millis() / 1000;
    if ((countMenuToReturn - countMenuEventStarted) >= MENUNOPRESSTIMER) {
      menuStages = 0;
    }
  }
  if (eventRead(&ev.bytes2)) {
    Serial.print(ev.type);
    Serial.println(ev.value);
    countMenuEventStarted = millis() / 1000;
    if (ev.value >= BUTTON_LONG_PRESS) {
      switch (ev.type) {
        case EVENEMENT_BUTTON_LEFT:
          ++menuStages == 4 ? menuStages = 0 : menuStages;
          Serial.println(menuStages);
          switch (menuStages) {
            case 1:
              prepareMenu12 = 1;
              break;
            case 2:
              prepareMenu23 = 1;
              if (prepareMenu12) {
                memcpy(&stateVal[1][0], &stateVal[0][0], MAXSHOWNVALUE);
                prepareMenu12 = 0;
              }
              break;
            case 3:
              if (prepareMenu23) {
                memcpy(&stateVal[0][0], &stateVal[1][0], MAXSHOWNVALUE);
                EEPROMwriteBytes(0, &stateVal[1][0], MAXSHOWNVALUE);
                prepareMenu23 = 0;
              }
              menuStages = 0;
              break;
          }
          break;
        case EVENEMENT_BUTTON_RIGHT:
          menuStages = 0;
          break;
        default: break;
      }
    }
    if (ev.value < BUTTON_LONG_PRESS) {
      switch (menuStages) {
        case 0: break;
        case 1:
          switch (ev.type) {
            case EVENEMENT_BUTTON_LEFT:
              ++setupPos == 4 ? setupPos = 0 : setupPos;
              break;
            case EVENEMENT_BUTTON_RIGHT:
              --setupPos == 255 ? setupPos = 3 : setupPos;
              break;
            default:
              break;
          }
          break;
        case 2:
          switch (ev.type) {
            case EVENEMENT_BUTTON_LEFT:
              ++stateVal[1][setupPos] == stateValMax[setupPos] + 1 ? stateVal[1][setupPos] = 0 : stateVal[1][setupPos];
              break;
            case EVENEMENT_BUTTON_RIGHT:
              --stateVal[1][setupPos] == 255 ? stateVal[1][setupPos] = stateValMax[setupPos] : stateVal[1][setupPos];
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
    }
  }
}
unsigned long printcount;
unsigned long printnow;
uint8_t isPrint, isFromPrint;
void LCD_blinkPrint(uint8_t isBlinc, uint8_t c, char* v) {
  if (isBlinc) {

    switch (isPrint) {
      case 0:
        lcd.print(v);
        printnow = millis() / 500;
        isPrint = 3;
        isFromPrint = 1;
        break;
      case 1:
        for (int i = 0; i < c; i++) {
          lcd.print(' ');
        }
        printnow = millis() / 500;
        isPrint = 3;
        isFromPrint = 0;
        break;
      case 3:
        printcount = millis() / 500;
        if ((printcount - printnow) >= PRINT_BLINK_TIME) {
          isPrint = isFromPrint;
        };
        break;
    }
  } else {
    lcd.print(v);
  }
}
void draw() {
  //setup selection
  for (int i = 0; i < 4; i++) {
    lcd.setCursor(1, i);
    if ((menuStages >= 1) && (setupPos == i))
      lcd.print(">");
    else
      lcd.print(" ");
  }
  //Air cond
  lcd.setCursor(2, 0);
  lcd.print("Air Cond Stat:");
  lcd.setCursor(16, 0);
  //lcd.print((char*)state[0]);
  LCD_blinkPrint((menuStages == 2) && (setupPos == 0), 3,
                 (char*)state[stateVal[(menuStages == 2) && (setupPos == 0)][0]]);
  // Temp
  lcd.setCursor(2, 1);
  lcd.print("Temp  :");
  lcd.setCursor(9, 1);
  //lcd.print(valTemp);
  char _vt[3];
  sprintf(_vt, "%d", stateVal[(menuStages == 2) && (setupPos == 1)][1]);
  LCD_blinkPrint((menuStages == 2) && (setupPos == 1), 2, _vt);
  lcd.setCursor(11, 1);
  lcd.print("C");
  //Recirc
  lcd.setCursor(2, 2);
  lcd.print("Recirc:");
  lcd.setCursor(9, 2);
  //lcd.print((char*)state[1]);
  LCD_blinkPrint((menuStages == 2) && (setupPos == 2), 3,
                 (char*)state[stateVal[(menuStages == 2) && (setupPos == 2)][2]]);
  //Fan
  lcd.setCursor(2, 3);
  lcd.print("Fan   :");
  lcd.setCursor(9, 3);
  //lcd.print((char*)state[2]);
  LCD_blinkPrint((menuStages == 2) && (setupPos == 3), 3,
                 (char*)state[stateVal[(menuStages == 2) && (setupPos == 3)][3]]);
}
void setup(void) {
  pinMode(b_left, INPUT_PULLUP);
  pinMode(b_right, INPUT_PULLUP);
  lcd.begin(20, 4);
  lcd.setCursor(3, 0);
  lcd.print("Temp control");
  lcd.setCursor(3, 2);
  lcd.print("By HerculeMania");
  lcd.setCursor(3, 3);
  lcd.print("version 1.00");
  lcd.clear();
  
  delay(2000);
  Serial.begin(9600);
  lcd.clear();
  //uint8_t eedata[4] = {1,20,1,1};
  //EEPROMwriteBytes(0, &eedata[0], 4)
  menuStages = 0;
  setupPos = 0;
  prepareMenu12 = 0;
  prepareMenu23 = 0;
  leftP = 1, rightP = 1;
  // pinMode(A0, INPUT);
  // pinMode(A1, INPUT);
  // pinMode(A2, INPUT);
  // pinMode(A3, INPUT);
  // pinMode(A6, INPUT);
  EEPROMreadBytes(0, &stateVal[0][0], MAXSHOWNVALUE);
  Wire.begin(DASH_ADDRESS);  // Slava adde = DASH_ADDRESS
  Wire.onRequest(requestEvent);
  systemOn =0;  
}
void loop(void) {
  draw();
  process_button(timeL, timeR);
  process_menu();
  prepareMessage();
}
void requestEvent() {
  Wire.write(txData, MESSAGE_DASH_RX_LENGTH);
}
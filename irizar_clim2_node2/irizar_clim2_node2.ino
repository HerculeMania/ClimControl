//#include <Wire.h>
#include <SoftwareWire.h>
#include "system.h"
byte txData[MESSAGE_RX_LENGTH];
byte rxData[MESSAGE_TX_LENGTH];

const uint8_t de0 = 11, de1 = 12, oe = 10;
SoftwareWire Wire(A4, A6);
typedef union _u {
  struct {
    uint8_t lowByte;
    uint8_t highByte;
  };
  uint16_t byte2;
} U2Bytes;
U2Bytes datasend;
void write16(uint8_t pin, uint8_t val) {
  if (pin < 10) {
    if (val) {
      datasend.lowByte |= 1 << (pin - 2);
    } else {
      datasend.lowByte &= ~(1 << (pin - 2));
    }
  } else {
    if (pin == 18) pin = 13;
    if (val) {
      datasend.highByte |= 1 << (pin - 10);
    } else {
      datasend.highByte &= ~(1 << (pin - 10));
    }
  }
}
void send16() {
  digitalWrite(de0, 1);
  digitalWrite(de1, 0);
  delay(0.1);
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(i + 2, datasend.lowByte & (1 << i));
  }
  digitalWrite(de0, 0);
  digitalWrite(de1, 1);
  delay(0.1);
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(i + 2, datasend.highByte & (1 << i));
  }
}
void setup() {

  // put your setup code here, to run once:
  for (uint8_t i = 2; i < 14; i++) {
    pinMode(i, OUTPUT);
  }
  Serial.begin(9600);
  digitalWrite(de0, 0);
  digitalWrite(de1, 0);
  digitalWrite(oe, 0);
  Serial.begin(9600);
  Serial.println("NODE 1:");
  Wire.begin(NODE_ADDRESS);  // Slava adde = NODE_ADDRESS
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

void loop() {
  for (int i = 0; i < MESSAGE_TX_LENGTH; i++) {
    if (i == 11) {
      digitalWrite(13, rxData[i]);
    } else {
      write16(i+2, rxData[i]);
    }
  }
  send16();
  for (int i = 0; i < MESSAGE_TX_LENGTH; i++) {
    Serial.print("pin");
    Serial.print(i+2);
    Serial.print("= ");
    Serial.print(rxData[i]);
    Serial.print(",");
  }
  Serial.println();
  txData[0] = 110;//map(analogRead(A0), 0, 1023, 0, MAXSENSOREAD);
  txData[1] = 100;//map(analogRead(A1), 0, 1023, 0, MAXSENSOREAD);
  txData[2] = 90;//map(analogRead(A2), 0, 1023, 0, MAXSENSOREAD);
  txData[3] = 0;//map(analogRead(A3), 0, 1023, 0, MAXSENSOREAD);
}
void receiveEvent(int numBytes) {
  for (int i = 0; i < numBytes && i < MESSAGE_TX_LENGTH; i++) {
    rxData[i] = Wire.read();
  }
  
  Serial.println("receiving.....");
}

void requestEvent() {
  Wire.write(txData, MESSAGE_RX_LENGTH);  
}

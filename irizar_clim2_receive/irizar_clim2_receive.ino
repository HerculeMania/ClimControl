#include <Wire.h>
#include "system.h"
#define DEBUG_SERIAL
//
byte txData[MESSAGE_TX_LENGTH];
byte rxData[MESSAGE_RX_LENGTH];
byte rxDataDash[MESSAGE_DASH_RX_LENGTH];
//

#define NODE1_CONN3_pin7_CirPumpV 4      //Circulation pmp Viking
#define NODE1_CONN3_pin10_BoostPump 0    //Booster pump
#define NODE1_CONN3_pin3_AuxHeater 5     //Auxilliary Heater unsuded
#define NODE1_CONN3_pin11_WhisHeatFan 1  //Whisper Heating fan (Rec. Blower)
//connector 2
#define NODE1_CONN2_pin8_ACCompClut 2   //AC Compr. Clutch
#define NODE1_CONN2_pin7_ACondFan 6     //AC Condenser fan
#define NODE1_CONN2_pin4_VentFanSp1 11  //Vent. fan speed 1
#define NODE1_CONN2_pin1_VentFanSp2 7   //Vent. fan speed 2
#define NODE1_CONN2_pin2_VentFanSp3 3   //Vent. fan speed 3
//connector 1
#define NODE1_CONN1_pin17_FlpFrshRecL0 12      // 6+8 Flap Fresh/Recirc, Ventilation air Left side
#define NODE1_CONN1_pin16_FlpFrshRecL1 8       //2+8 Flap Fresh/Recirc, Ventilation air Left side
#define NODE1_CONN1_pin14_FlpFrshRecR0 13      //7+8 Flap Fresh/Recirc, Ventilation air Right side
#define NODE1_CONN1_pin13_FlpFrshRecR1 9       //3+8Flap Fresh/Recirc, Ventilation air Right side
#define NODE1_CONN1_pin11_ValConvCirc0 14      //8+8 Valve, Convector circuit
#define NODE1_CONN1_pin10_ValConvCirc1 10      //4+8 Valve, Convector circuit
#define NODE1_CONN1_pin7_ValveVentCircuit0 15  //9+8 Valve, Ventilation circuit
#define NODE1_CONN1_pin4_ValveVentCircuit1 16  //8+5 Valve, Ventilation circuit
const uint8_t
  //connector 3 driver side 9pins
  pinHConn_1_RcircBlow = 2,     //Recirculation blower for guide
  pinHConn3_2_BoosterPump = 3,  //Booster pump
  pinHConn3_3_AuxHeat = 4,      //Auxilliary Heater

  //connector 2 driver side 12pin  s
  pinHConn2_4_DefFanSpd2 = 9,   //Defroster fan speed 2
  pinHConn2_5_DefFanSpd3 = 10,  //Defroster fan speed 3
  pinHConn2_7_ACConFan = 7,     //AC Condenser fan
  pinHConn2_10_ACCompClut = 6,  //AC Compressor Clutch
  pinHConn2_11_WhHeatFan = 5,   //Whisper heating fan (Rec. Blower)
  //connector 1 driver side 18pins
  pinHConn3_4_VlvConvCir = 8,       //Valve, Driver’s convector circuit
  pinHConn3_16_FlpFloorScrn1 = 11,  //Flap Floor/Screen
  pinHConn3_13_FlpFloorScrn0 = 12,  //Flap Floor/Screen
  pinHConn3_7_FLpFrshDef0 = 13,     //Flap Fresh/Recirc, defroster air
  pinHConn3_10_FLpFrshDef1 = A0;    //Flap Fresh/Recirc defroster air
#define NODE1_IN_TEMP_SENSOR_COMP 3
#define NODE1_IN_TEMP_SENSOR_FEED_WATER 0
#define NODE1_IN_TEMP_SENSOR_VENT_AIR 2
#define NODE1_IN_AC_HP_LP 1
#define NODE0_IN_TEMP_SENSOR_DEF_AIR 4  //Temp sensor, Defroster air
#define NODE0_IN_TEMP_SENSOR_OUT_AIR 5  //Temp sensor, Outside air
#define NODE0_IN_TEMP_SENSOR_DRV_PLC 6  //Temp sensor, Driver’s place
#define NODE0_IN_AC_HP 8
#define NODE0_IN_AC_LP 7
#define NODE0_AC_SET_STATE 0
#define NODE0_AC_SET_TEMP 1
#define NODE0_AC_SET_FAN 2
#define NODE0_AC_SET_REC 3
#define NODE0_AC_IS_ON 9
typedef union _u {
  struct {
    uint8_t lowByte;
    uint8_t highByte;
  };
  uint16_t byte2;
} U2Bytes;
U2Bytes datasend;
void setup() {
  for (int i = 2; i < 15; i++) {
    pinMode(i, OUTPUT);  //init all thes pins as output;
  }
  Wire.begin();  // Master
#if defined DEBUG_SERIAL
  Serial.begin(9600);
  Serial.println("RECEIVE:");
#endif
  //delay(5000);
}
unsigned long previousMillis = 0;
const long interval = 100;  // 1 second

uint8_t blink;
unsigned long countBlink;
byte checksum = 0;
uint8_t tempGap;
void controlAirConditioningSystem(uint8_t currentCabinTemp, uint8_t targetTemp,
                                  uint8_t wichAC, uint8_t recirc, uint8_t fan) {
  bool coolingNeeded = currentCabinTemp > targetTemp;
  if (coolingNeeded) {
    if (wichAC == 2) {
      txData[NODE1_CONN2_pin8_ACCompClut] = 1;
      txData[NODE1_CONN2_pin7_ACondFan] = 1;
      tempGap = currentCabinTemp - targetTemp;
      if (tempGap > 4) {
        txData[NODE1_CONN2_pin2_VentFanSp3] = 1;
        txData[NODE1_CONN2_pin1_VentFanSp2] = 0;
      } else {
        if (fan)
          txData[NODE1_CONN2_pin2_VentFanSp3] = 1;
        else txData[NODE1_CONN2_pin2_VentFanSp3] = 0;
        txData[NODE1_CONN2_pin2_VentFanSp3] = 0;
        txData[NODE1_CONN2_pin1_VentFanSp2] = 1;
      }
    }

    digitalWrite(pinHConn2_10_ACCompClut, 0);
    digitalWrite(pinHConn2_7_ACConFan, 0);
    digitalWrite(pinHConn2_5_DefFanSpd3, 0);
    digitalWrite(pinHConn2_4_DefFanSpd2, 0);
    digitalWrite(pinHConn2_4_DefFanSpd2, 0);
    digitalWrite(pinHConn3_2_BoosterPump, 0);
    digitalWrite(pinHConn3_10_FLpFrshDef1, 0);
    digitalWrite(pinHConn3_2_BoosterPump, 0);
    digitalWrite(pinHConn3_7_FLpFrshDef0, 0);
    digitalWrite(pinHConn3_10_FLpFrshDef1, 0);
    txData[NODE1_CONN3_pin10_BoostPump] = 0;
    txData[NODE1_CONN3_pin7_CirPumpV] = 0;
    txData[NODE1_CONN3_pin11_WhisHeatFan] = 0;
  } else {
    txData[NODE1_CONN2_pin8_ACCompClut] = 0;
    txData[NODE1_CONN2_pin7_ACondFan] = 0;
    if (fan)
      txData[NODE1_CONN2_pin2_VentFanSp3] = 1;
    else txData[NODE1_CONN2_pin2_VentFanSp3] = 0;
    txData[NODE1_CONN2_pin1_VentFanSp2] = 0;
    if (targetTemp - currentCabinTemp > 4) {
      digitalWrite(pinHConn2_10_ACCompClut, 1);
      digitalWrite(pinHConn2_7_ACConFan, 1);
      digitalWrite(pinHConn2_5_DefFanSpd3, 1);
      digitalWrite(pinHConn2_4_DefFanSpd2, 1);
      digitalWrite(pinHConn2_4_DefFanSpd2, 1);
      digitalWrite(pinHConn3_2_BoosterPump, 1);
      digitalWrite(pinHConn3_10_FLpFrshDef1, 1);
      digitalWrite(pinHConn3_2_BoosterPump, 1);
      digitalWrite(pinHConn3_7_FLpFrshDef0, 1);
      digitalWrite(pinHConn3_10_FLpFrshDef1, 0);
      txData[NODE1_CONN3_pin10_BoostPump] = 1;
      txData[NODE1_CONN3_pin7_CirPumpV] = 1;
      txData[NODE1_CONN3_pin11_WhisHeatFan] = 1;
    } else {

      digitalWrite(pinHConn2_10_ACCompClut, 0);
      digitalWrite(pinHConn2_7_ACConFan, 0);
      digitalWrite(pinHConn2_5_DefFanSpd3, 0);
      digitalWrite(pinHConn2_4_DefFanSpd2, 0);
      digitalWrite(pinHConn2_4_DefFanSpd2, 0);
      digitalWrite(pinHConn3_2_BoosterPump, 0);
      digitalWrite(pinHConn3_10_FLpFrshDef1, 0);
      digitalWrite(pinHConn3_2_BoosterPump, 0);
      digitalWrite(pinHConn3_7_FLpFrshDef0, 0);
      digitalWrite(pinHConn3_10_FLpFrshDef1, 0);
      txData[NODE1_CONN3_pin10_BoostPump] = 0;
      txData[NODE1_CONN3_pin7_CirPumpV] = 0;
      txData[NODE1_CONN3_pin11_WhisHeatFan] = 0;
    }
  }
  if (recirc) {

    txData[NODE1_CONN1_pin13_FlpFrshRecR1] = 1;
    txData[NODE1_CONN1_pin14_FlpFrshRecR0] = 0;
    txData[NODE1_CONN1_pin17_FlpFrshRecL0] = 0;
    txData[NODE1_CONN1_pin16_FlpFrshRecL1] = 1;
  } else {

    txData[NODE1_CONN1_pin13_FlpFrshRecR1] = 0;
    txData[NODE1_CONN1_pin14_FlpFrshRecR0] = 1;
    txData[NODE1_CONN1_pin17_FlpFrshRecL0] = 1;
    txData[NODE1_CONN1_pin16_FlpFrshRecL1] = 0;
  }
  digitalWrite(pinHConn3_3_AuxHeat, 0);
}
// ------Tension aux bornes de R2 :
// V_t=5\cdot \frac{\mathrm{ADC}}{255}-
// ------Résistance R2 via pont diviseur :
// R_2=\frac{V_t\cdot R_1}{V-V_t},\quad R_1=4700\, \Omega ,\; V=5\, \mathrm{V}-
// ------Température en Kelvin (modèle β) :
// T(K)=\frac{\beta }{\ln \! \left( \frac{R_2}{R_{25}}\right) +\frac{\beta }{T_{25}}}- avec R_{25}=4700\, \Omega , \beta =3950\, K, T_{25}=298\, K.
// ------Température en Celsius :
// T(\degree C)=T(K)-273.15



uint8_t computeTemp(uint8_t temp) {
  float Vout = temp * 5.0 / 255.0;
  float Rfixed = 4700.0;
  float Rthermistor = (Rfixed * Vout) / (5.0 - Vout);

  // Beta model parameters
  float T0 = 298.15;  // 25°C in Kelvin
  float R0 = 4700.0;
  float beta = 3950.0;

  //float tempK = 1.0 / (1.0 / T0 + (1.0 / beta) * log(Rthermistor / R0));
  float tempK = beta / (log(Rthermistor / R0) + (beta / T0));
  uint8_t tempC = (uint8_t)tempK - 273.15;
  return tempC;
}
void prepareMessage() {
  uint8_t tempC = computeTemp(rxData[NODE1_IN_TEMP_SENSOR_COMP]);
  uint8_t tempV = computeTemp(rxData[NODE1_IN_TEMP_SENSOR_VENT_AIR]);
  uint8_t tempW = computeTemp(rxData[NODE1_IN_TEMP_SENSOR_FEED_WATER]);
#if defined DEBUG_SERIAL
  Serial.print("Temp Compartement:");
  Serial.println(tempC);
  Serial.print("Temp Air Vent:");
  Serial.println(tempV);
  Serial.print("Temp Feed Water:");
  Serial.println(tempW);
  Serial.print("Compressor cluch is ");
  if (rxData[NODE1_IN_AC_HP_LP])
    Serial.println("ON");
  else
    Serial.println("OFF");
#endif
  // put either tempC ot tempV for the sensor you want as Thermostat function.
  controlAirConditioningSystem(tempC, 22, 2, 1, 1);
}
void loop() {
  prepareMessage();
  Wire.requestFrom(NODE_ADDRESS, MESSAGE_RX_LENGTH);
  uint8_t i = 0;
  while (Wire.available() && i < MESSAGE_RX_LENGTH) {
    rxData[i++] = Wire.read();
  }
#if defined DEBUG_SERIAL
  Serial.println("Received from Node");
  for (int i = 0; i < MESSAGE_RX_LENGTH; i++) {
    Serial.print(rxData[i]);
    Serial.print(" ");
  }
  Serial.println();
#endif
  //   Wire.requestFrom(DASH_ADDRESS, MESSAGE_DASH_RX_LENGTH);
  //   i = 0;
  //   while (Wire.available() && i < MESSAGE_DASH_RX_LENGTH) {
  //     rxDataDash[i++] = Wire.read();
  //   }
  // #if defined DEBUG_SERIAL
  //   Serial.println("Received from Dash");
  //   for (int i = 0; i < MESSAGE_DASH_RX_LENGTH; i++) {
  //     Serial.print(rxDataDash[i]);
  //     Serial.print(" ");
  //   }
  //   Serial.println();
  // #endif
  Wire.beginTransmission(NODE_ADDRESS);
  Wire.write(txData, MESSAGE_TX_LENGTH);
  Wire.endTransmission();
}

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "LittleFS.h"                                                   // ✅ Use LittleFS
#include <Preferences.h>                                                // ✅ Preferences for persistent storage
#define MESSAGE_RX_LENGTH 4                                             // receive from node
#define MESSAGE_TX_LENGTH 17                                            //send to node
#define MAXSHOWNVALUE 4                                                 // setup values in the driver dashboard
#define MAX_DRIVER_SENSORIN 5                                           // number of sensor in the driver side
#define MESSAGE_DASH_RX_LENGTH MAX_DRIVER_SENSORIN + MAXSHOWNVALUE + 1  //plus system ON
#define BUTTON_LONG_PRESS 5
#define PRINT_BLINK_TIME 1
#define MENUNOPRESSTIMER 10
#define DASH_ADDRESS 10                  //address of the Dashboard
#define NODE_ADDRESS 8                   //address of the node
#define MAXSENSOREAD 255                 // max value of sensor read
#define MAXSETUPTEMPVAL 300              // time for menu with no press to go back to normal
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
//
#define NODE1_IN_TEMP_SENSOR_COMP 3
#define NODE1_IN_TEMP_SENSOR_FEED_WATER 0
#define NODE1_IN_TEMP_SENSOR_VENT_AIR 2
#define NODE1_IN_AC_HP_LP 1
//
// ESP32 creates its own WiFi network
const char* ssid = "scania_wifi";
const char* password = "012345678";

WebServer server(80);
Preferences prefs;  // ✅ Preferences object

// State variables
int fanSpeed;    //= 0;
int targetTemp;  //= 24;
bool recirc;     //= false;
bool systemOn;   //= true;
int readTemp;    //= 0;
int readtempVent = 0;
int readTempWater = 0;
int isClutch = 0;
byte txData[MESSAGE_TX_LENGTH];
byte rxData[MESSAGE_RX_LENGTH];

uint8_t computeTemp(uint8_t temp, float Rfixed) {
  float Vout = temp * 5.0 / 255.0;
  //float Rfixed = 4700.0;
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

void printStatus() {
  Serial.println("=== Climate Control Status ===");
  Serial.printf("Fan Speed: %d\n", fanSpeed);
  Serial.printf("Target Temp: %d\n", targetTemp);
  Serial.printf("Recirculation: %s\n", recirc ? "ON" : "OFF");
  Serial.printf("System: %s\n", systemOn ? "ON" : "OFF");
  Serial.printf("Current Compartment Temp: %s\n", String(readTemp));
  Serial.printf("Current Water Temp: %s \n", String(readTempWater));
  Serial.printf("Current Vent Temp: %s \n", String(readtempVent));
  Serial.printf("Current Compressor state High/Low pressure: %s \n", isClutch ? "ON" : "OFF");
  Serial.println("------data to send to the node 2-----");
  for (int i = 0; i < MESSAGE_TX_LENGTH; i++) {
    switch (i) {
      case 0: Serial.print("C3_p10_BoostPump: "); break;
      case 1: Serial.print("C3_p11_WhisHeatFan: "); break;
      case 2: Serial.print("C2_p8_ACCompClut: "); break;
      case 3: Serial.print("C2_p2_VentFanSp3: "); break;
      case 4: Serial.print("C3_p7_CirPumpV: "); break;
      case 5: Serial.print("C3_p3_AuxHeater: "); break;
      case 6: Serial.print("C2_p7_ACondFan: "); break;
      case 7: Serial.print("C2_p1_VentFanSp2: "); break;
      case 8: Serial.print("C1_p16_FlpFrshRecL1: "); break;
      case 9: Serial.print("C1_p13_FlpFrshRecR1: "); break;
      case 10: Serial.print("C1_p10_ValConvCirc1: "); break;
      case 11: Serial.print("C2_p4_VentFanSp1: "); break;
      case 12: Serial.print("C1_p17_FlpFrshRecL0: "); break;
      case 13: Serial.print("C1_p14_FlpFrshRecR0: "); break;
      case 14: Serial.print("C1_p11_ValConvCirc0: "); break;
      case 15: Serial.print("C1_p7_ValveVentCircuit0: "); break;
      case 16: Serial.print("C1_p4_ValveVentCircuit1: "); break;
      default: break;
    }
    Serial.println(txData[i] == 0 ? "OFF" : "ON");
  }
  Serial.println("-------------------------------------");
  Serial.println("==============================");
}

// --- I2C helpers ---
// void sendToNode() {
//   Wire.beginTransmission(NODE_ADDRESS);
//   Wire.write(txData, MESSAGE_TX_LENGTH);
//   Wire.endTransmission();
// }
// //
// void receiveFromNode() {
//   Wire.requestFrom(NODE_ADDRESS, MESSAGE_RX_LENGTH);
//   uint8_t i = 0;
//   while (Wire.available() && i < MESSAGE_RX_LENGTH) {
//     rxData[i++] = Wire.read();
//   }
// }

void receiveEvent(int numBytes) {
  for (int i = 0; i < numBytes && i < MESSAGE_RX_LENGTH; i++) {
    rxData[i] = Wire.read();
  }
  //Serial.println("receiving.....");
}

void requestEvent() {
  Wire.write(txData, MESSAGE_TX_LENGTH);
  //Serial.println("sending data");
}

void readTemperature() {
  readTemp = computeTemp(rxData[NODE1_IN_TEMP_SENSOR_COMP], 4700.0);
  readtempVent = computeTemp(rxData[NODE1_IN_TEMP_SENSOR_VENT_AIR], 4700.0);
  readTempWater = computeTemp(rxData[NODE1_IN_TEMP_SENSOR_FEED_WATER], 4700.0);
  isClutch = rxData[NODE1_IN_AC_HP_LP];
}

// --- LittleFS file listing ---
void listLittleFSFiles() {
  Serial.println("Listing LittleFS files:");
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.print("FILE: ");
    Serial.print(file.name());
    Serial.print("  SIZE: ");
    Serial.println(file.size());
    file = root.openNextFile();
  }
}

// --- Web routes ---
void handleRoot() {
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    Serial.println("index.html not found in LittleFS!");
    server.send(404, "text/plain", "index.html not found");
    return;
  }
  Serial.println("Serving index.html");
  server.streamFile(file, "text/html");
  file.close();
}

void handleFileRequest() {
  String path = server.uri();
  if (!path.startsWith("/")) path = "/" + path;

  if (LittleFS.exists(path)) {
    String contentType = "text/plain";
    if (path.endsWith(".html")) contentType = "text/html";
    else if (path.endsWith(".css")) contentType = "text/css";
    else if (path.endsWith(".js")) contentType = "application/javascript";
    else if (path.endsWith(".svg")) contentType = "image/svg+xml";
    else if (path.endsWith(".ttf")) contentType = "font/ttf";

    File file = LittleFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    Serial.println("Served file: " + path);
    return;
  }

  server.send(404, "text/plain", "File Not Found: " + path);
  Serial.println("File not found: " + path);
}

void handleFan() {
  if (server.hasArg("value")) {
    fanSpeed = server.arg("value").toInt();
    prefs.begin("climate", false);
    prefs.putInt("fanSpeed", fanSpeed);
    prefs.end();
    printStatus();
  }
  server.send(200, "text/plain", "OK");
}

void handleTemp() {
  if (server.hasArg("set")) {
    targetTemp = server.arg("set").toInt();
    prefs.begin("climate", false);
    prefs.putInt("targetTemp", targetTemp);
    prefs.end();
    printStatus();
  }
  server.send(200, "text/plain", "OK");
}

void handleRecirc() {
  if (server.hasArg("state")) {
    recirc = (server.arg("state") == "on");
    prefs.begin("climate", false);
    prefs.putBool("recirc", recirc);
    prefs.end();
    printStatus();
  }
  server.send(200, "text/plain", "OK");
}

void handleSystem() {
  if (server.hasArg("state")) {
    systemOn = (server.arg("state") == "on");
    prefs.begin("climate", false);
    prefs.putBool("systemOn", systemOn);
    prefs.end();
    printStatus();
  }
  server.send(200, "text/plain", "OK");
}

void handleGetTemp() {
  String t = String(readTemp);
  server.send(200, "text/plain", t);
  //Serial.printf("sent temp : %s \n", t);
}
void handleGetState() {
  String json = "{";
  json += "\"systemOn\":" + String(systemOn ? "true" : "false") + ",";
  json += "\"fanSpeed\":" + String(fanSpeed) + ",";
  json += "\"targetTemp\":" + String(targetTemp) + ",";
  json += "\"recirc\":" + String(recirc ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}
void setup() {
  Serial.begin(115200);
  Wire.begin(NODE_ADDRESS);  // ESP32 as I2C master
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }
  // Restore saved state
  prefs.begin("climate", true);
  systemOn = prefs.getBool("systemOn", false);
  fanSpeed = prefs.getInt("fanSpeed", 0);
  targetTemp = prefs.getInt("targetTemp", 20);
  recirc = prefs.getBool("recirc", false);
  prefs.end();
  printStatus();

  // Show all files in LittleFS
  //listLittleFSFiles();

  // Create WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("ESP32 Access Point started");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // Register routes
  server.on("/", handleRoot);
  server.on("/fan", handleFan);
  server.on("/temp", handleTemp);
  server.on("/recirc", handleRecirc);
  server.on("/system", handleSystem);
  server.on("/getTemp", handleGetTemp);
  server.on("/getState", handleGetState);

  // ✅ Catch-all for static files (CSS, JS, SVG, fonts)
  server.onNotFound(handleFileRequest);

  server.begin();
}
void recirculationfunc() {
  if (recirc) {
    txData[NODE1_CONN1_pin13_FlpFrshRecR1] = 0;
    txData[NODE1_CONN1_pin14_FlpFrshRecR0] = 1;
    txData[NODE1_CONN1_pin16_FlpFrshRecL1] = 0;
    txData[NODE1_CONN1_pin17_FlpFrshRecL0] = 1;
  } else {
    txData[NODE1_CONN1_pin13_FlpFrshRecR1] = 1;
    txData[NODE1_CONN1_pin14_FlpFrshRecR0] = 0;
    txData[NODE1_CONN1_pin16_FlpFrshRecL1] = 1;
    txData[NODE1_CONN1_pin17_FlpFrshRecL0] = 0;
  }
}
int isVent = 0;
void tempfunc() {
  if ((readTemp > targetTemp)) {
    txData[NODE1_CONN2_pin8_ACCompClut] = 1;
    txData[NODE1_CONN2_pin7_ACondFan] = 1;
  } else {
    txData[NODE1_CONN2_pin8_ACCompClut] = 0;
    txData[NODE1_CONN2_pin7_ACondFan] = 0;
  }
}
void ventfunc() {
  switch (fanSpeed) {
    case 0:
      txData[NODE1_CONN2_pin4_VentFanSp1] = 0;
      txData[NODE1_CONN2_pin1_VentFanSp2] = 0;
      txData[NODE1_CONN2_pin2_VentFanSp3] = 0;
      //isVent = 0;
      break;
    case 50:
      txData[NODE1_CONN2_pin4_VentFanSp1] = 0;
      txData[NODE1_CONN2_pin1_VentFanSp2] = 1;
      txData[NODE1_CONN2_pin2_VentFanSp3] = 0;
      //isVent = 1;
      break;
    case 100:
      txData[NODE1_CONN2_pin4_VentFanSp1] = 0;
      txData[NODE1_CONN2_pin1_VentFanSp2] = 0;
      txData[NODE1_CONN2_pin2_VentFanSp3] = 1;
      //isVent = 1;
      break;
  }
}
void HVACfunc() {
  recirculationfunc();
  tempfunc();
  ventfunc();
}
void loop() {
  server.handleClient();
  readTemperature();
  if (systemOn) {
    HVACfunc();
  } else {
    for (int i = 0; i < MESSAGE_TX_LENGTH; i++) {
      txData[i] = 0;
    }
  }
}

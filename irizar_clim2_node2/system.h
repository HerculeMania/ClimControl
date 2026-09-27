#ifndef SYSTEM_H
#define SYSTEM_H
#define MESSAGE_RX_LENGTH 4 // receive from node
#define MESSAGE_TX_LENGTH 17 //send to node
#define MAXSHOWNVALUE 4 // setup values in the driver dashboard
#define MAX_DRIVER_SENSORIN 5 // number of sensor in the driver side 
#define MESSAGE_DASH_RX_LENGTH MAX_DRIVER_SENSORIN+MAXSHOWNVALUE+1//plus system ON
#define BUTTON_LONG_PRESS 5
#define PRINT_BLINK_TIME 1
#define MENUNOPRESSTIMER 10
#define DASH_ADDRESS 10 //address of the Dashboard
#define NODE_ADDRESS 8 //address of the node 
#define MAXSENSOREAD 255 // max value of sensor read
#define MAXSETUPTEMPVAL 300 // time for menu with no press to go back to normal
#endif //end of SYSTEM_H
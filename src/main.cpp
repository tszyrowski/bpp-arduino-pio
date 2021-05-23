
#include <Arduino.h>
#include <ArduinoBLE.h>
#include <U8g2lib.h>
//#include <Arduino_LSM9DS1.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

#define BUFF_LENGTH 60        // Number of messages stored in queue
#define MSG_LEENGTH 20        // length of the string consttructed for BLE
#define REPORTING_PERIOD 1000 // how often values are reported
#define OFFLINE_REPORTING_PERIOD 10000 // how often values are reported

/* Device name which can be scene in BLE scanning software. */
#define BLE_DEVICE_NAME "Arduino Nano 33 BLE OXY"
/* Local name which should pop up when scanning for BLE devices. */
#define BLE_LOCAL_NAME "bpp-oxy"


//*******Public Variables*******
char currMsg[MSG_LEENGTH + 1];  // array to store data line (and \0 string char)
char ringBuffer[BUFF_LENGTH][MSG_LEENGTH];
int buffOldestIdx = 0;
int buffSampleNum = 0;

//*******Public Classes*******
//BLEDevice central;
BLEService customService("1101");
BLECharacteristic bleMsg("2101", BLERead | BLENotify, MSG_LEENGTH);
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);
PulseOximeter pox;
// create service

//*******Public Functions*******
void initialDisplay(void);
void updateDisplay(int accelX, int accelY);
void blinkLED(void);
void updateDisplay(int HR,int SPO);
void createMsg(char * newMsg,int HR,int SPO);
int send_BLE(void);
void pushBuffer(char* newMsg);
int popBuffer(char* newMsg);

void arrayReplace(char* pBuffIdx, char* newMsg, int msgLenght);

// Callback (registered below) fired when a pulse is detected
void onBeatDetected()
{
  digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Beat!");
    u8g2.setFont(u8g2_font_cursor_tr);
    u8g2.setCursor(118,10);
    u8g2.print("_");
    u8g2.sendBuffer();
  digitalWrite(LED_BUILTIN, HIGH);
}

void setup() {
  //goTime = millis();
  Serial.begin(9200);
  // while(!Serial);                       // wait with everything for serial
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Now is in setup ");

  //IMU.begin();
  u8g2.begin();
  initialDisplay();
  Serial.println("Dispaly should be initialised !!!");

  // INITIALISE BLE
  if (!BLE.begin()) {
    Serial.println("BLE failed to Initiate");
    while (1);
  }

  String address = BLE.address();
  Serial.print("Local address is: ");
  Serial.println(address);                    // display local addres

  BLE.setDeviceName(BLE_DEVICE_NAME);
  BLE.setLocalName(BLE_LOCAL_NAME);
  // set the UUID for the service this peripheral advertises: 
  BLE.setAdvertisedService(customService);
  customService.addCharacteristic(bleMsg);
  BLE.addService(customService);
  char firstRead[] = {"xxxxxxxxxxxxxxxxxxxx"};
  bleMsg.setValue((unsigned char *)firstRead,21);
  // Start advertising BLE.  It will start continuously transmitting BLE
  //   advertising packets and will be visible to remote BLE central devices
  //   until it receives a new connection 
  BLE.advertise();

  Serial.println("Bluetooth device is now active, waiting for connections...");

  if (!pox.begin()) {
    Serial.println("POX INIT FAILED");
    while(1);
  } else {
    Serial.println("POX INIT SUCCESS");
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_20_8MA);
  // Register a callback for the beat detection
  pox.setOnBeatDetectedCallback(onBeatDetected);

}

void loop() {
  static uint32_t POX_period=millis()+25;
  static uint32_t BLE_period=millis()+50;
  static uint32_t REPORT_period=millis()+75;
  static uint32_t DISPLAY_period=millis()+100;
  static int BLE_connected;
  
  if (millis() >= REPORT_period){
    if(BLE_connected)
      REPORT_period += REPORTING_PERIOD;
    else
      REPORT_period += OFFLINE_REPORTING_PERIOD;
    
    //read pox values
    int HRclean = pox.getHeartRate();
    int SpO2 = pox.getSpO2();
    if((HRclean>30 && HRclean<220 && SpO2>30 && SpO2<100))
    {
      createMsg(currMsg,HRclean,SpO2);
      pushBuffer(currMsg);
      Serial.printf("##Number of samples in buffer: %i\n", buffSampleNum);
      Serial.print("Heart rate:");
      Serial.print(HRclean);
      Serial.print("bpm / SpO2:");
      Serial.print(SpO2);
      Serial.println("%");
    }
  }

  if (millis() >= DISPLAY_period){
    DISPLAY_period += REPORTING_PERIOD;

    //read pox values
    int HRclean = pox.getHeartRate();
    int SpO2 = pox.getSpO2();
    updateDisplay(HRclean,SpO2);
  }

  if (millis() >= BLE_period){
    BLE_period += 100;
    BLEDevice central = BLE.central();
    if(central.connected() && bleMsg.subscribed()){
      if(BLE_connected==0){ //add .5s delay if was disconnected
        BLE_period += 500;
      }
      else{
        //send data through BLE
        send_BLE();
      }
      BLE_connected=1;
    }
    else{
      BLE_connected=0;
    }
  }
  
  if (millis() >= POX_period){
    POX_period += 10;
    // need to update HR and Oxy readings
    pox.update();
  }
  
}

void pushBuffer(char* newMsg)
{
   // add message to buffer
   int i = (buffOldestIdx+buffSampleNum)%BUFF_LENGTH;
    arrayReplace(ringBuffer[i], newMsg, MSG_LEENGTH);
    buffSampleNum++;    //increase number of samples stored
    if (buffSampleNum > BUFF_LENGTH){  // if buffer is full
      buffOldestIdx++;                     // delate oldest sample
      buffSampleNum--;                    // threfore decrease number of samples
      if (buffOldestIdx == BUFF_LENGTH){    // if get to the end of buffer
        buffOldestIdx = 0;                  // move index to the buff begining
      }
    }
}

int popBuffer(char* newMsg)
{
  if(buffSampleNum>0)
  {
      arrayReplace(                     // get the oldest message to be send
        newMsg, ringBuffer[buffOldestIdx], MSG_LEENGTH
      );
      buffSampleNum--;                     // decrease number of samples
      buffOldestIdx++;                     // delate oldest sample
      if (buffOldestIdx == BUFF_LENGTH){   // if idx get to the end of buffer 
        buffOldestIdx = 0;                 // start from 1st to catch up
      }
      return 1;
  }
  return 0;
}

int send_BLE(void) {
  //BLEDevice central = BLE.central();        // listen for BLE peripherals to connect:
  char sendMsg[MSG_LEENGTH+1];
  //if(central.connected() && bleMsg.subscribed()){
    if(popBuffer(sendMsg)){  // 
    // char mockMsg[20] = {"HELLO world from pi"};
      sendMsg[MSG_LEENGTH] = '\0';      // make it a string
      Serial.printf("##Msg to  central: %s\n", sendMsg);  // consume message
      // Send data over bluetooth
      // bleMsg.setValue((unsigned char *)mockMsg,20);
      bleMsg.setValue((unsigned char *)sendMsg,20);
    }
    //return 1;
  //}
  return 0;
}

void arrayReplace(char* pBuffIdx, char* newMsg, int msgLenght){
  for(int i=0; i < msgLenght; i++){
    pBuffIdx[i] = newMsg[i];
    // *(pBuffIdx + i) = *(newMsg + i);
  }
}

void createMsg(char * newMsg,int HR,int SPO) {
  // Modify the messsge ti include time and most recent readings
  unsigned long allSeconds=millis()/1000;
  int runHours= allSeconds/3600;
  int secsRemaining=allSeconds%3600;
  int runMinutes=secsRemaining/60;
  int runSeconds=secsRemaining%60;
  sprintf(
    newMsg, "R%03d:%02d:%02d,%03d,%03d;",
    runHours, runMinutes, runSeconds,HR,SPO
  );
}

void updateDisplay(int HR,int SPO)
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_crox2h_tr);
  u8g2.setCursor(0,12);  
  u8g2.print("HR");
  u8g2.setCursor(75,12);  
  u8g2.print("Bpm");
  u8g2.setCursor(0,30);
  u8g2.print("SpO2 ");
  u8g2.setCursor(75,30);
  u8g2.print("%"); 
  u8g2.setFont(u8g2_font_fub11_tf); 
  u8g2.setCursor(45,12);  
  u8g2.print(HR);
  u8g2.setCursor(45,30);  
  u8g2.print(SPO);
  u8g2.setFont(u8g2_font_cursor_tr);
  u8g2.setCursor(118,10);
  u8g2.print("^");
  u8g2.sendBuffer();
}

void initialDisplay(void) {
  static bool displayInitialised = false;
  if (not displayInitialised){
    u8g2.clearBuffer();
    u8g2.setCursor(0, 12);
    u8g2.setFont(u8g2_font_crox2hb_tr);
    u8g2.print("BPP Plymouth");
    u8g2.setFont(u8g2_font_crox2h_tr);
    u8g2.setCursor(30, 29);
    u8g2.print("Initialising...");
    u8g2.sendBuffer();
    delay(4000);
    displayInitialised = true;

    u8g2.clearBuffer();

    // after all works
    u8g2.setCursor(20, 12);
    u8g2.print("Initiliased");
    u8g2.setCursor(0, 29);
    u8g2.print("You can start read");
    u8g2.sendBuffer();
  }
}
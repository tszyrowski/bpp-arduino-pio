#include <Arduino.h>
#include <ArduinoBLE.h>
#include <U8g2lib.h>
#include <Arduino_LSM9DS1.h>

#define BUFF_LENGTH 60        // Number of messages stored in queue
#define MSG_LEENGTH 20        // length of the string consttructed for BLE
#define REPORTING_PERIOD 1000 // how often values are reported

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);

int accelX = 1;
int accelY = 1;
float x, y ,z;
char currMsg[MSG_LEENGTH + 1];  // array to store data line (and \0 string char)
bool displayInitialised = false;
char ringBuffer[BUFF_LENGTH][MSG_LEENGTH];
int buffCurrIdx = 0;

int msgSendIdx = 0;  // Index of last message send from ring buffer
// scheduler
long unsigned int goTime;
// LED state
bool ledState = true;

BLEService accelerometerService("1101");
BLECharacteristic xyMsg("2101", BLERead | BLENotify, MSG_LEENGTH);

void initialDisplay() {
  if (not displayInitialised){
    u8g2.clearBuffer();
    u8g2.setCursor(0, 12);
    u8g2.setFont(u8g2_font_crox2hb_tr);
    u8g2.print("ACCELEROMETR");
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

void updateDisplay(int accelX, int accelY){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_crox2hb_tr);
  u8g2.setCursor(0, 12);
  u8g2.print("X:");
  u8g2.setCursor(75, 12);
  u8g2.print("Y:");
  u8g2.setFont(u8g2_font_fub11_tf);
  u8g2.setCursor(0, 30);
  u8g2.print(accelX);
  u8g2.setCursor(75, 30);
  u8g2.print(accelY);
  u8g2.sendBuffer();
}

void blinkLED() {
  // simple blinking LED every time report should be
  goTime = millis() + REPORTING_PERIOD;
  if (ledState) {
    digitalWrite(LED_BUILTIN, HIGH);
    // Serial.print("Works all ok the LED is ON ");
    // Serial.println(millis());
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    // Serial.print("The LED is OFF ");
    // Serial.println(millis());
  }
  ledState = !ledState;
}

void updateXY(int* new_x, int* new_y) {
  // function read the x and y passed as pointers and update values
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
    *new_x = (1+x)*100;
    *new_y = (1+y)*100;
  } else {
    *new_x = 0;
    *new_y = 0;
  }
}

void getMsg() {
  // Modify the messsge ti include time and most recent readings
  unsigned long allSeconds=millis()/1000;
  int runHours= allSeconds/3600;
  int secsRemaining=allSeconds%3600;
  int runMinutes=secsRemaining/60;
  int runSeconds=secsRemaining%60;
  sprintf(
    currMsg, "R%03d:%02d:%02d,%02d,%03d;",
    runHours, runMinutes, runSeconds, accelX, accelY
  );
}

void arrayReplace(char* pBuffIdx, char* newMsg, int msgLenght){
  for(int i=0; i < msgLenght; i++){
    pBuffIdx[i] = newMsg[i];
    // *(pBuffIdx + i) = *(newMsg + i);
  }
}

void msgConsumer(){
  char sendMSg[MSG_LEENGTH+1];
  int i = 0;
  if (Serial){                          // if consumer is ready
    Serial.printf(
      "enetered consumer i is %d msgSendIdx is %d buffCurrIdx is %d\n",
      i, msgSendIdx, buffCurrIdx
    );
    while (msgSendIdx != buffCurrIdx){  // the producer idx in front of consumer
      arrayReplace(                     // get the message to be send
        sendMSg, ringBuffer[msgSendIdx], MSG_LEENGTH
      );
      sendMSg[MSG_LEENGTH] = '\0';      // make it a string
      Serial.printf("## %d: %s\n", i, sendMSg);  // consume message
      msgSendIdx++;                     // update mesg idx to the next slot
      if (msgSendIdx == BUFF_LENGTH){   // if idx get to the end of buffer 
        msgSendIdx = 0;                 // start from 1st to catch up
      }
      i++;
    }
  }
}

void setup() {
  goTime = millis();
  Serial.begin(9200);
  while(!Serial);                       // wait with everything for serial
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Now is in setup ");
  IMU.begin();
  u8g2.begin();
  initialDisplay();
  Serial.println("Dispaly should be initialised !!!");
}

void loop() {
  if (millis() >= goTime){
    Serial.printf(
      "** start loop: msgSendIdx is %d buffCurrIdx is %d\n",
      msgSendIdx, buffCurrIdx
    );
    blinkLED();
    updateXY(&accelX, &accelY);  // new valules are read
    updateDisplay(accelX, accelY);

    getMsg();                          // the message is updated
    // add message to buffer
    arrayReplace(ringBuffer[buffCurrIdx], currMsg, MSG_LEENGTH);
    buffCurrIdx++;                    // Update buffer index to next slot
    if (buffCurrIdx == BUFF_LENGTH){  // if index excceds the buffer length
      buffCurrIdx = 0;                // get index back to front of quees
    }
    Serial.printf(
      "--- In loop: msgSendIdx is %d buffCurrIdx is %d\n",
      msgSendIdx, buffCurrIdx
    );
    // CONSUME MESSAGE
    msgConsumer();
    // Move consumer index by one forward so there are always last BUFF_LENGTH
    // samples available.
    if (buffCurrIdx == msgSendIdx - 1){  // if all messages are consumed
      msgSendIdx++;                      // update the consimer index
      if (msgSendIdx == BUFF_LENGTH){    // if get to the end of buffer
        msgSendIdx = 0;                  // move index to the buff begining
      }
    }
    Serial.printf(
      "++ END loop: msgSendIdx is %d buffCurrIdx is %d\n",
      msgSendIdx, buffCurrIdx
    );
  }
}

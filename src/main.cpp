#include <Arduino.h>
#include <ArduinoBLE.h>
#include <U8g2lib.h>
#include <Arduino_LSM9DS1.h>

#define MSG_LEENGTH 20        // length of the string consttructed for BLE
#define REPORTING_PERIOD 1000 // how often values are reported

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);

int accelX = 1;
int accelY = 1;
float x, y ,z;
char currentReading[MSG_LEENGTH];  // array to store data line
bool displayInitialised = false;

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
    Serial.print("Works all ok the LED is ON ");
    Serial.println(millis());
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("The LED is OFF ");
    Serial.println(millis());
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

void setup() {
  // put your setup code here, to run once:
  goTime = millis();
  Serial.begin(9200);
  while(!Serial);  // wait with everything for serial
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Now is in setup ");

  IMU.begin();

  u8g2.begin();
  initialDisplay();
  Serial.println("Dispaly should be initialised !!!");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (millis() >= goTime){
    blinkLED();
    updateXY(&accelX, &accelY);
    Serial.print(accelX);
    Serial.print(' ');
    Serial.println(accelY);
    updateDisplay(accelX, accelY);
  } 
}
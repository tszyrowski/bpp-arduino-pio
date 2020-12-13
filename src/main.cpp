#include <Arduino.h>
#include <ArduinoBLE.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9200);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("Works all ok ");
  Serial.println(millis());
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  Serial.print("Switching off LED ");
  Serial.println(millis());
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
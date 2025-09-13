#include <Arduino.h>
#include <M5Unified.h>
#include <SD.h>
#include "Stackchan_servo.h"
#include <Avatar.h>

using namespace m5avatar;

Avatar avatar;
StackchanSERVO servo;

void setup() {
  // M5Stack initialization
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Log.setLogLevel(m5::log_target_display, ESP_LOG_INFO);
  M5.Log.setEnableColor(m5::log_target_serial, false);
  
  // SD card initialization
  SD.begin(GPIO_NUM_4, SPI, 25000000);
  delay(2000);
  
  Serial.println("Stackchan Main - Starting initialization...");
  
  // Basic servo initialization (default pins and settings)
  // X-axis: pin 21, start position 90°, offset 0
  // Y-axis: pin 22, start position 90°, offset 0
  servo.begin(21, 90, 0, 22, 90, 0, SG90);
  delay(2000);
  
  // Avatar initialization
  avatar.init();
  
  Serial.println("Stackchan Main - Initialization complete!");
  Serial.println("Ready for operation...");
}

void loop() {
  // Main loop for stackchan
  static unsigned long lastMoveTime = 0;
  static int moveStep = 0;
  
  // Simple demonstration movement every 5 seconds
  if (millis() - lastMoveTime > 5000) {
    lastMoveTime = millis();
    
    switch (moveStep) {
      case 0:
        Serial.println("Main: Looking around - Right");
        servo.motion(70, 90);
        break;
      case 1:
        Serial.println("Main: Looking around - Center");
        servo.motion(90, 90);
        break;
      case 2:
        Serial.println("Main: Looking around - Left");
        servo.motion(110, 90);
        break;
      case 3:
        Serial.println("Main: Looking around - Center");
        servo.motion(90, 90);
        break;
    }
    
    moveStep = (moveStep + 1) % 4;
  }
  
  // Handle M5Stack button presses
  M5.update();
  if (M5.BtnA.wasPressed()) {
    Serial.println("Button A pressed - Center position");
    servo.motion(90, 90);
  }
  if (M5.BtnB.wasPressed()) {
    Serial.println("Button B pressed - Nod");
    servo.motion(90, 80);
    delay(500);
    servo.motion(90, 90);
  }
  if (M5.BtnC.wasPressed()) {
    Serial.println("Button C pressed - Shake head");
    servo.motion(80, 90);
    delay(300);
    servo.motion(100, 90);
    delay(300);
    servo.motion(90, 90);
  }
  
  delay(50);
}

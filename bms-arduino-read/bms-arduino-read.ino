#include "BatteryInterface.hpp"

#define MAX485_RE_DE 45

BatteryInterface2 battery_interface_(Serial3, MAX485_RE_DE);
BatteryInfo batt_info_;

void setup() {
  // For battery, RS485
  pinMode(MAX485_RE_DE, OUTPUT);   // RE and DE connected to Pin 45
  Serial3.begin(9600, SERIAL_8N1); //TX[16] to DI and RX[17] to RO
  battery_interface_.init();

  // For printing on Serial Monitor
  Serial.begin(57600);
}

void loop() {
  static unsigned long last_battery_update_time = 0;
  int state_ = battery_interface_.queryMon2();
  switch (state_) {
    case 0:
      batt_info_ = battery_interface_.getBattInfo();
      Serial.print("Battery Current[0.1A]: "); Serial.println(batt_info_.batt_current);
      Serial.print("Battery Voltage[0.01mV]: "); Serial.println(batt_info_.batt_voltage);
      delay(500); // poll again in 0.5s
      break;
    case 1:
      delay(5); // to send 6 bytes on 9600 baud ~ 6ms, so call again in ~5ms, and block 1s
      // for such a blocking example, skipping the above delay(5) is more than fine
      last_battery_update_time = millis();
      break;
    case 2:
      delay(30); // 27 bytes on 9600 baud ~ 27ms
      break;
  }
  // if no reply for some time, purge
  if ((millis() - last_battery_update_time) > 1000) {
    // not connected, data mismatch, or data loss
    Serial.println("Battery read timeout");
    battery_interface_.abortLastCall();
    delay(5000); // don't keep firing
  }
}

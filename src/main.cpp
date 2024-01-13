#include <ESPDateTime.h>

#include "provision.h"
#include "mqtt.h"

WiFiClient wifiClient;

std::shared_ptr<RadarMqtt> mqtt;
std::shared_ptr<SettingsManager> settings;

unsigned long lastInvokeTime = 0; // Store the last time you called the function
const unsigned long dayMillis = 24UL * 60 * 60 * 1000; // Milliseconds in a day

void setup() {
  Serial.begin(115200);
  // esp_wifi_set_ps(WIFI_PS_NONE);
  delay(2000);
  Serial.printf("Hello\n");
  //reset_provisioning();
  wifi_connect();

  settings = std::make_shared<SettingsManager>();

  DateTime.setTimeZone(settings->tz.c_str());
  DateTime.begin(/* timeout param */);
  lastInvokeTime = millis();
  if (!DateTime.isTimeValid()) {
    Serial.printf("Failed to get time from server\n");
  }

  mqtt = std::make_shared<RadarMqtt>(settings);
}

// int ii;

void loop() {
  unsigned long currentMillis = millis();
  
  mqtt->handle();

  if (currentMillis - lastInvokeTime >= dayMillis) {
      DateTime.begin(1000);
      lastInvokeTime = currentMillis;
  }
}

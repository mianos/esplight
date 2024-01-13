#include <ESPDateTime.h>
#include <SPI.h> 
#include <Wire.h>

#include <Adafruit_TCS34725.h>

#include "provision.h"
#include "mqtt.h"

WiFiClient wifiClient;

std::shared_ptr<LuxMqtt> mqtt;
std::shared_ptr<SettingsManager> settings;
std::unique_ptr<TwoWire> wireInstance;
std::unique_ptr<Adafruit_TCS34725> tcs;

unsigned long lastInvokeTime = 0; // Store the last time you called the function
const unsigned long dayMillis = 24UL * 60 * 60 * 1000; // Milliseconds in a day
const int LED_PIN = GPIO_NUM_18;

void setup() {
  pinMode(LED_PIN, OUTPUT);  // Set the LED pin as an output
  digitalWrite(LED_PIN, HIGH);  // Set the LED pin to LOW (LED off)

  Serial.begin(115200);
  // esp_wifi_set_ps(WIFI_PS_NONE);

  delay(2000);
  //reset_provisioning();
  digitalWrite(LED_PIN, LOW);  // Set the LED pin to LOW (LED off)
  wifi_connect();
  settings = std::make_shared<SettingsManager>();

 
  wireInstance = std::make_unique<TwoWire>(0);
  wireInstance->begin(33, 35);
  tcs = std::make_unique<Adafruit_TCS34725>(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

  if (tcs->begin(TCS34725_ADDRESS, wireInstance.get())) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
  }


  DateTime.setTimeZone(settings->tz.c_str());
  DateTime.begin(/* timeout param */);
  lastInvokeTime = millis();
  if (!DateTime.isTimeValid()) {
    Serial.printf("Failed to get time from server\n");
  }

  mqtt = std::make_shared<LuxMqtt>(settings, std::move(tcs));
}

// int ii;

void loop() {
  unsigned long currentMillis = millis();
  
  mqtt->handle();

  if (currentMillis - lastInvokeTime >= dayMillis) {
      DateTime.begin(1000);
      lastInvokeTime = currentMillis;
  }
  delay(10);
}

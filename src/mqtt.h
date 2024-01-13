#pragma once
#include <memory>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_TCS34725.h>

#include "settings.h"

struct LuxMqtt {
  WiFiClient espClient;
  PubSubClient client;
  std::shared_ptr<SettingsManager> settings;
  std::unique_ptr<Adafruit_TCS34725> tcs;
  unsigned long lastLuxCalculation = 0;


  LuxMqtt(std::shared_ptr<SettingsManager> settings, std::unique_ptr<Adafruit_TCS34725> tcs);
  void callback(char* topic_str, byte* payload, unsigned int length);
  void calculate_lux();

  bool reconnect();
  void handle();
};


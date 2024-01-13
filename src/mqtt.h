#pragma once
#include <memory>
#include <WiFi.h>
#include <PubSubClient.h>

#include "settings.h"

struct RadarMqtt {
  WiFiClient espClient;
  PubSubClient client;
  std::shared_ptr<SettingsManager> settings;

  void callback(char* topic_str, byte* payload, unsigned int length);
  RadarMqtt(std::shared_ptr<SettingsManager> settings);

  bool reconnect();
  void handle();
};


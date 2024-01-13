#pragma once
#include <vector>
#include <SPIFFS.h>
#include <ArduinoJson.h>

struct SettingsManager {
  String mqttServer = "mqtt2.mianos.com";
  int mqttPort = 1883;
  String sensorName = "light";
  String tz = "AEST-10AEDT,M10.1.0,M4.1.0/3";
  int notify = 60;

  SettingsManager(); // Constructor declaration

  void fillJsonDocument(JsonDocument& doc);
  void printSettings();
  enum class SettingChange {
    None = 0,
    //VolumeChanged
  };
  std::vector<SettingChange> loadFromDocument(JsonDocument& doc);
};

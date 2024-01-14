#include <unordered_map>
#include <memory>
#include <sstream>

#include <ESPDateTime.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <StringSplitter.h>

#include "provision.h"

#include "mqtt.h"

const int ONBOARD_LED_PIN = GPIO_NUM_18;
const int FLOOD_PIN = GPIO_NUM_37;

static std::unordered_map<std::string, int> pinMap = {
  {"onboard_led", ONBOARD_LED_PIN},
  {"flood", FLOOD_PIN}
};

void LuxMqtt::publish_error(const std::string& error) {
    JsonDocument doc;
    doc["time"] = DateTime.toISOString();
    doc["error"] = error;
    String topic = "tele/" + settings->sensorName + "/error";
    String output;
    serializeJson(doc, output);
    client.publish(topic.c_str(), topic.c_str());
}


void LuxMqtt::setPin(const std::string& pinName, bool state) {
    auto it = pinMap.find(pinName);
    if (it != pinMap.end()) {
        digitalWrite(it->second, state);
    } else {
      std::ostringstream stream;
      stream << "Error, unknown pin '" << pinName << "'";
      publish_error(stream.str());
    }
}


void LuxMqtt::callback(char* topic_str, byte* payload, unsigned int length) {
  auto topic = String(topic_str);
  auto splitter = StringSplitter(topic, '/', 4);
  auto itemCount = splitter.getItemCount();
  if (itemCount < 3) {
    Serial.printf("Item count less than 3 %d '%s'\n", itemCount, topic_str);
    return;
  }
#if 1
  for (auto i = 0; i < itemCount; i++) {
    String item = splitter.getItemAtIndex(i);
    Serial.println("Item @ index " + String(i) + ": " + String(item));
  }
  Serial.printf("command '%s'\n", splitter.getItemAtIndex(itemCount - 1).c_str());
#endif
  
  if (splitter.getItemAtIndex(0) == "cmnd") {
    JsonDocument jpl;
    auto err = deserializeJson(jpl, payload, length);
    if (err) {
      Serial.printf("deserializeJson() failed: '%s'\n", err.c_str());
      return;
    }
    String output;
    serializeJson(jpl, output);
    auto dest = splitter.getItemAtIndex(itemCount - 1);
    if (dest == "reprovision") {
        Serial.printf("clearing provisioning\n");
        reset_provisioning();
    } else if (dest == "restart") {
        Serial.printf("rebooting\n");
        ESP.restart();
    } else if (dest == "switch") {
      for (JsonPair kv : jpl.as<JsonObject>()) {
        setPin(kv.key().c_str(), kv.value().as<bool>());
      }
    } else if (dest == "settings") {
      auto result = settings->loadFromDocument(jpl);
//      if (std::find(result.begin(), result.end(), SettingsManager::SettingChange::VolumeChanged) != result.end()) {
//        volume.setVolume(settings->volume / 100);
//        prev_volume = volume.volume();  // if volume is set during a play, go back to the one set by settings.
//      }
      settings->printSettings();
    }
  }
}


LuxMqtt::LuxMqtt(std::shared_ptr<SettingsManager> settings, std::unique_ptr<Adafruit_TCS34725> tcs)
    : settings(settings), tcs(std::move(tcs)), client(espClient) {

  client.setBufferSize(1024);
  client.setServer(settings->mqttServer.c_str(), settings->mqttPort);
  Serial.printf("init mqtt, server '%s'\n", settings->mqttServer.c_str());
  for (const auto& pair : pinMap) {
    pinMode(pair.second, OUTPUT); // Assuming these are output pins
  }
  client.setCallback([this](char* topic_str, byte* payload, unsigned int length) {
    callback(topic_str, payload, length);
  });
}


bool LuxMqtt::reconnect() {
  String clientId =  WiFi.getHostname(); // name + '-' + String(random(0xffff), HEX);
  Serial.printf("Attempting MQTT connection... to %s name %s\n", settings->mqttServer.c_str(), clientId.c_str());
  if (client.connect(clientId.c_str())) {
    delay(1000);
    String cmnd_topic = String("cmnd/") + settings->sensorName + "/#";
    client.subscribe(cmnd_topic.c_str());
    Serial.printf("mqtt connected as sensor '%s'\n", settings->sensorName.c_str());

    JsonDocument doc;
    doc["version"] = 2;
    doc["time"] = DateTime.toISOString();
    doc["hostname"] = WiFi.getHostname();
    doc["ip"] = WiFi.localIP().toString();
    settings->fillJsonDocument(doc);
    String status_topic = "tele/" + settings->sensorName + "/init";
    String output;
    serializeJson(doc, output);
    client.publish(status_topic.c_str(), output.c_str());
    return true;
  } else {
    Serial.printf("failed to connect to %s port %d state %d\n", settings->mqttServer.c_str(), settings->mqttPort, client.state());
    delay(10000);
    return false;
  }
}


void LuxMqtt::calculate_lux(uint16_t& lux, uint16_t& colorTemp, uint16_t& r, uint16_t& g, uint16_t& b, uint16_t& c) {
  tcs->getRawData(&r, &g, &b, &c);
  colorTemp = tcs->calculateColorTemperature_dn40(r, g, b, c);
  lux = tcs->calculateLux(r, g, b);
}

void LuxMqtt::outputValues(uint16_t lux, uint16_t colorTemp, uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
    JsonDocument doc;
    doc["time"] = DateTime.toISOString();
    doc["lux"] = lux;
    doc["colour_temp"] = colorTemp; 
    String status_topic = "tele/" + settings->sensorName + "/ctlux";
    String output;
    serializeJson(doc, output);
    client.publish(status_topic.c_str(), output.c_str());
}

void LuxMqtt::handle() {
    if (!client.connected()) {
      if (!reconnect()) {
        return;
      }
    }
    uint16_t lux, colorTemp, r, g, b, c;
    calculate_lux(lux, colorTemp, r, g, b, c);

    auto currentMillis = millis();
    bool luxChanged = abs(static_cast<int>(lux) - static_cast<int>(lastLux)) > settings->lux_change;
    bool colorTempChanged = abs(static_cast<int>(colorTemp) - static_cast<int>(lastColorTemp)) > settings->colour_temp_change;
    bool shouldOutput = luxChanged || colorTempChanged || currentMillis - lastLuxCalculation >= settings->notify * 1000;

    if (shouldOutput) {
        outputValues(lux, colorTemp, r, g, b, c);
        lastLux = lux;
        lastColorTemp = colorTemp;
        lastLuxCalculation = currentMillis;
    }
    client.loop();  // mqtt client loop
}

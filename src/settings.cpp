#include "settings.h"


template<typename T>
bool getConfigOrDefault(JsonDocument& doc, const char* key, T& value) {
    T oldValue = value;
    if (doc.containsKey(key)) {
        value = doc[key].as<T>();
    }
#if 0
    else {
        Serial.printf("%s not found in config. Using default.\n", key);
    }
#endif
    return value != oldValue;
}

SettingsManager::SettingsManager() {
    if (!SPIFFS.begin(true)) {
        Serial.printf("An error occurred while mounting SPIFFS\n");
        return;
    }

    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
        Serial.printf("Failed to open config file. Loading default settings\n");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        Serial.println("Failed to deserialize config file. Loading default settings\n");
    } else {
        serializeJson(doc, Serial);
        loadFromDocument(doc);
        printSettings();
    }
    configFile.close();
}

std::vector<SettingsManager::SettingChange> SettingsManager::loadFromDocument(JsonDocument& doc) {
    std::vector<SettingChange> changes;
//    if (getConfigOrDefault(doc, "volume", volume)) {
//        changes.push_back(SettingChange::VolumeChanged);
//    }
    getConfigOrDefault(doc, "mqtt_server", mqttServer);
    getConfigOrDefault(doc, "mqtt_port", mqttPort);
    getConfigOrDefault(doc, "sensor_name", sensorName);
    getConfigOrDefault(doc, "tz", tz);
    getConfigOrDefault(doc, "notify", notify);
    getConfigOrDefault(doc, "lux_change", lux_change);
    getConfigOrDefault(doc, "colour_temp_change", colour_temp_change);
    return changes;
}

// Method to fill a JsonDocument with current settings
void SettingsManager::fillJsonDocument(JsonDocument& doc) {
    doc["mqtt_server"] = mqttServer;
    doc["mqtt_port"] = mqttPort;
    doc["sensor_name"] = sensorName;
    doc["tz"] = tz;
    doc["notify"] = notify;
    doc["lux_change"] = lux_change;
    doc["colour_temp_change"] = colour_temp_change;
}

void SettingsManager::printSettings() {
    Serial.printf("MQTT Server: %s\n", mqttServer.c_str());
    Serial.printf("MQTT Port: %d\n", mqttPort);
    Serial.printf("Sensor Name: %s\n", sensorName.c_str());
    Serial.printf("Timezone: %s\n", tz.c_str());
    Serial.printf("Notify: %d seconds\n", notify);
    Serial.printf("Lux change: %d\n", lux_change);
    Serial.printf("Colour temp change: %d\n", colour_temp_change);
}

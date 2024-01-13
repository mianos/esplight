#include <ESPDateTime.h>
#include <SPI.h> 
#include <Wire.h>

#include <Adafruit_TCS34725.h>

#include "provision.h"
#include "mqtt.h"

WiFiClient wifiClient;

std::shared_ptr<RadarMqtt> mqtt;
std::shared_ptr<SettingsManager> settings;
TwoWire wireInstance = TwoWire(0);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

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

 
  wireInstance.begin(33, 35); // GPIO_NUM_16, GPIO_NUM_17); 

  if (tcs.begin(TCS34725_ADDRESS, &wireInstance)) {
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

  uint16_t r, g, b, c, colorTemp, lux;

  tcs.getRawData(&r, &g, &b, &c);
  // colorTemp = tcs.calculateColorTemperature(r, g, b);
  colorTemp = tcs.calculateColorTemperature_dn40(r, g, b, c);
  lux = tcs.calculateLux(r, g, b);

  Serial.print("Color Temp: "); Serial.print(colorTemp, DEC); Serial.print(" K - ");
  Serial.print("Lux: "); Serial.print(lux, DEC); Serial.print(" - ");
  Serial.print("R: "); Serial.print(r, DEC); Serial.print(" ");
  Serial.print("G: "); Serial.print(g, DEC); Serial.print(" ");
  Serial.print("B: "); Serial.print(b, DEC); Serial.print(" ");
  Serial.print("C: "); Serial.print(c, DEC); Serial.print(" ");
  Serial.println(" ");
  delay(100);
}

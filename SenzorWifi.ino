#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- CONFIG: rulăm prin cablu (USB) fără WiFi/MQTT
#define LINK_USB_ONLY 1

// --- Pini senzori
#define ONE_WIRE_BUS        15
#define SOIL_MOISTURE_PIN   32
#define SOIL_MOISTURE_PIN_2 35
#define MQ1_PIN             33
#define MQ2_PIN             34
#define MQ3_PIN             39

// --- setări (păstrate ca în codul tău, nefolosite în modul USB)
const char* WIFI_SSID = "Orange-b39kTz-2G";
const char* WIFI_PASS = "K22ZyGy994x9AC6FUN";
const char* MQTT_HOST = "192.168.1.34";
const int   MQTT_PORT = 1883;
const char* MQTT_USER = "";
const char* MQTT_PASS = "";
const char* MQTT_CLIENT_ID = "ESP32";
const char* TOPIC_CHAT  = "ESP32";
const char* TOPIC_AVAIL = "esp32/availability";
const char* TOPIC_STATE = "esp32/state";

#if !LINK_USB_ONLY
  WiFiServer   server(80);
  WiFiClient   net;
  PubSubClient mqtt(net);
#endif

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

unsigned long lastPublish = 0;

// ---------- UTIL: citire linie din Serial (pt. „chat”) ----------
bool readSerialLine(String &out) {
  static String buf;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') { out = buf; buf = ""; return true; }
    if (c != '\r') buf += c;
  }
  return false;
}

#if !LINK_USB_ONLY
// ===== MQTT callback (NEFOLOSIT în modul USB) =====
void mqttCallback(char* topic, byte* payload, unsigned int length) {}
#endif

void setup() {
  Serial.begin(115200);
  sensors.begin();
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(SOIL_MOISTURE_PIN_2, INPUT);

#if !LINK_USB_ONLY
  // (rămâne aici dacă vei reveni la WiFi/MQTT)
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(300); }
#endif

  Serial.println(F("[ESP32] USB mode ready"));
}

void loop() {
  // ---- Citiri senzori
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  int   raw1 = analogRead(SOIL_MOISTURE_PIN);
  int   raw2 = analogRead(SOIL_MOISTURE_PIN_2);
  int   v1   = analogRead(MQ1_PIN);
  int   v2   = analogRead(MQ2_PIN);
  int   v3   = analogRead(MQ3_PIN);

  float moisture1    = map(raw1, 4095, 0, 0, 100);
  float moisture2    = map(raw2, 4095, 0, 0, 100);
  float meanMoisture = (moisture1 + moisture2) / 2.0;
  float co2avg       = (v1 + v2 + v3) / 3.0;

  // ---- Trimite JSON pe USB, o dată la 1s (linie per mesaj)
  unsigned long now = millis();
  if (now - lastPublish > 1000) {
    lastPublish = now;
    StaticJsonDocument<256> st;
    st["temperature"]  = temperatureC;
    st["moisture1"]    = moisture1;
    st["moisture2"]    = moisture2;
    st["meanMoisture"] = meanMoisture;
    st["co2_lvl1"]     = v1;
    st["co2_lvl2"]     = v2;
    st["co2_lvl3"]     = v3;
    st["co2_avg"]      = co2avg;

    String line;  // line-delimited JSON pentru Web Serial / Serial Monitor
    serializeJson(st, line);
    Serial.println(line);
  }

  // ---- „Chat” pe USB: ce primește pe linie, răspunde cu [ESP32] …
  String cmd;
  if (readSerialLine(cmd) && cmd.length()) {
    Serial.print("[ESP32] ");
    Serial.println(cmd);
  }

  delay(10);
}

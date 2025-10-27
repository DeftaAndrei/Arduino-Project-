#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
// 2) Maparea pinilor (senzori)
#define ONE_WIRE_BUS 15
#define SOIL_MOISTURE_PIN 32
#define SOIL_MOISTURE_PIN_2 35
#define MQ1_PIN 33 // Nivel 1 jos
#define MQ2_PIN 34 //D34 Nivel 2 mijloc
#define MQ3_PIN 39 // nivel 3 cel mai sus
// 3) Config Wi-Fi
const char* ssid = "DIGI-6yNU";
const char* password = "Tj3ug9wTPw";
//4) MQTT ADD
const char* MQTT_HOST ="192.168.1.137"; // IP de pe Rasbbery
const int MQTT_PORT = 1883;
const char* MQTT_USER = ""; // ex. "espuser"
const char* MQTT_PASS = ""; // ex. "parola"
93
const char* MQTT_CLIENT_ID = "esp32-sensor-01";
const char* TOPIC_AVAIL = "automatizare/esp32/availability";
const char* TOPIC_STATE = "automatizare/esp32/senzori/state";
const char* TOPIC_CMD = "automatizare/esp32/cmd";
WiFiServer server(80);
WiFiClient net ;
PubSubClient mqtt(net); // Enter MQTT ADD
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
void setup() {
Serial.begin(115200);
WiFi.begin(ssid, password);
Serial.print("Conectare la WiFi");
while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}
Serial.println("\nConectat la WiFi!");
Serial.print("IP ESP32: ");
Serial.println(WiFi.localIP());
server.begin();
sensors.begin();
pinMode(SOIL_MOISTURE_PIN, INPUT);
pinMode(SOIL_MOISTURE_PIN_2, INPUT);
}
void loop() {
sensors.requestTemperatures();
float temperatureC = sensors.getTempCByIndex(0);
int raw1 = analogRead(SOIL_MOISTURE_PIN);
int raw2 = analogRead(SOIL_MOISTURE_PIN_2);
int valoare1 = analogRead(MQ1_PIN);
int valoare2 = analogRead(MQ2_PIN);
int valoare3 = analogRead(MQ3_PIN);
float moisture1 = map(raw1, 4095, 0, 0, 100);
94
float moisture2 = map(raw2, 4095, 0, 0, 100);
float meanMoisture = (moisture1 + moisture2) / 2;
float Co2=(valoare1 + valoare2 + valoare3) / 3 ;
// Afiseaza valorile in Serial Monitor
Serial.print("Temperatura: ");
Serial.print(temperatureC);
Serial.print(" °C, Umiditate1: ");
Serial.print(moisture1);
Serial.print("%, Umiditate2: ");
Serial.print(moisture2);
Serial.print("%, Medie: ");
Serial.print(meanMoisture);
Serial.println("%");
Serial.println("Senzor 1 CO2 nivel 1 ");Serial.println(valoare1);
Serial.println("Senzor 2 CO2 nivel 2 ");Serial.println(valoare2);
Serial.println("Senzor 3 Co2 nivel 3");Serial.println(valoare3);
Serial.println("Valoare Medie Co2 :") ; Serial.println(Co2);
WiFiClient client = server.available();
if (client) {
client.setTimeout(2000); // Timeout pentru client
StaticJsonDocument<256> doc;
doc["temperature"] = temperatureC;
doc["meanMoisture"] = meanMoisture;
String jsonString;
serializeJson(doc, jsonString);
client.println("HTTP/1.1 200 OK");
client.println("Content-Type: application/json");
client.println("Connection: close");
client.println();
client.println(jsonString);
client.stop();}
delay(1000)}

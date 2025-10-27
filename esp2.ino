#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>


const char* ssid = "Orange-b39kTz-2G";           // <- SSID-ul rețelei tale
const char* password = "K22ZyGy994x9AC6FUN";     // <- Parola rețelei tale

WebServer server(80);

void handleRoot() {
  if (LittleFS.exists("/index.html")) {
    File file = LittleFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(500, "text/plain", "Fisierul index.html nu a fost gasit.");
  }
}

void handleLedOn() {
  digitalWrite(2, HIGH);
  server.send(200, "text/plain", "LED aprins");
}

void handleLedOff() {
  digitalWrite(2, LOW);
  server.send(200, "text/plain", "LED stins");
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConectat la IP: " + WiFi.localIP().toString());

  if (!LittleFS.begin()) {
    Serial.println("LittleFS nu a putut fi inițializat!");
    return;
  }

  pinMode(2, OUTPUT);

  server.on("/", handleRoot);
  server.on("/led/on", handleLedOn);
  server.on("/led/off", handleLedOff);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Pagina nu exista");
  });

  server.begin();
}

void loop() {
  server.handleClient();
}
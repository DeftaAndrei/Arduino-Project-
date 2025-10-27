// Mega2560 <-> ESP-01 (AT)  — status conexiune Wi-Fi in Serial Monitor
#define ESP     Serial1
#define ESP_BAUD 115200

const char* SSID = "Orange-b39kTz-2G";
const char* PASS = "K22ZyGy994x9AC6FUN";

String readESP(uint32_t tout = 3000) {
  uint32_t t = millis(); String r;
  while (millis() - t < tout) {
    while (ESP.available()) r += (char)ESP.read();
  }
  return r;
}

bool sendAT(const String& cmd, uint32_t tout = 3000) {
  ESP.println(cmd);
  String r = readESP(tout);
  Serial.print(">> "); Serial.println(cmd);
  Serial.print("<< "); Serial.println(r);
  if (r.indexOf("FAIL") != -1 || r.indexOf("ERROR") != -1) return false;
  return (r.indexOf("OK") != -1) || (r.indexOf("WIFI GOT IP") != -1) || (r.indexOf("WIFI CONNECTED") != -1);
}

bool wifiConnect(const char* ssid, const char* pass) {
  if (!sendAT("AT", 1000)) return false;
  sendAT("AT+CWMODE=1", 1000);
  // Join AP (poate dura mult)
  ESP.print("AT+CWJAP=\""); ESP.print(ssid); ESP.print("\",\""); ESP.print(pass); ESP.println("\"");
  String r = readESP(25000);
  Serial.print("<< "); Serial.println(r);
  if (r.indexOf("FAIL") != -1 || r.indexOf("DISCONNECT") != -1 || r.indexOf("ERROR") != -1) return false;
  return (r.indexOf("WIFI GOT IP") != -1) || (r.indexOf("OK") != -1);
}

bool wifiIsConnected() {
  ESP.println("AT+CWJAP?");
  String r = readESP(1500);
  Serial.print("<< "); Serial.println(r);
  return r.indexOf("+CWJAP:\"") != -1;   // Răspuns conține SSID dacă e conectat
}

void printIP() {
  ESP.println("AT+CIFSR");
  String r = readESP(1500);
  Serial.print("IP info: ");
  Serial.println(r);
}

void setup() {
  Serial.begin(115200);
  ESP.begin(ESP_BAUD);
  Serial.println("Conectare ESP la Wi-Fi...");

  bool ok = wifiConnect(SSID, PASS);
  if (ok && wifiIsConnected()) {
    Serial.println("✅ Conectat la Wi-Fi.");
    printIP();   // vei vedea STAIP=...
  } else {
    Serial.println("❌ NU s-a conectat la Wi-Fi.");
  }
}

void loop() {
  // opțional: poți re-verifica periodic statusul
  // static uint32_t last=0; if (millis()-last>10000){ last=millis(); Serial.println(wifiIsConnected()?"[OK] conectat":"[X] deconectat"); }
}

// Mega2560 <-> ESP-01 (AT) — auto-baud + diagnostic conectare
#define ESP Serial1      // TX1/RX1 -> ESP (pinii 18/19)
#define DBG Serial       // USB -> Serial Monitor

const char* SSID = "Orange-b39kTz-2G";
const char* PASS = "K22ZyGy994x9AC6FUN";

const long BAUDS[] = {115200, 57600, 38400, 19200, 9600};

String readESP(uint32_t tout=3000) {
  String r; uint32_t t0 = millis();
  while (millis() - t0 < tout) while (ESP.available()) r += (char)ESP.read();
  return r;
}
bool sendAT(const String& cmd, uint32_t tout=3000) {
  ESP.println(cmd);
  String r = readESP(tout);
  DBG.print(">> "); DBG.println(cmd);
  DBG.print("<< "); DBG.println(r);
  return r.indexOf("OK") != -1;
}
long detectBaud() {
  for (long b : BAUDS) {
    ESP.begin(b); delay(50);
    ESP.println("AT");
    if (readESP(400).indexOf("OK") != -1) { DBG.print("[BAUD] "); DBG.println(b); return b; }
  }
  return -1;
}
int parseCwJapError(const String& r) {
  int p = r.indexOf("+CWJAP:");
  if (p != -1) return r.substring(p+7).toInt();
  if (r.indexOf("FAIL")  != -1) return 4;
  if (r.indexOf("ERROR") != -1) return -1;
  return 0;
}
void explainError(int code) {
  switch(code) {
    case 1: DBG.println("Timeout asociere (semnal slab)."); break;
    case 2: DBG.println("Parola gresita."); break;
    case 3: DBG.println("SSID inexistent/ascuns."); break;
    case 4: DBG.println("Eroare generica (FAIL)."); break;
    case 201: DBG.println("WPA handshake fail (crypto)."); break;
    case 202: DBG.println("AP nu raspunde."); break;
    default: DBG.print("Cod necunoscut: "); DBG.println(code);
  }
}
void setup() {
  DBG.begin(115200);
  DBG.println("\n[ESP pe Serial1] Detectez baud...");
  long b = detectBaud();
  if (b < 0) { DBG.println("Nu raspunde la AT. Verifica 3.3V, EN=3.3V, RX/TX, level shifting."); return; }
  ESP.begin(b);
  sendAT("ATE0", 500);
  sendAT("AT+CWMODE=1", 1000);

  ESP.println("AT+CWJAP?");
  DBG.print("Status: "); DBG.println(readESP(1500));

  DBG.println("[Wi-Fi] Conectare...");
  ESP.print("AT+CWJAP=\""); ESP.print(SSID); ESP.print("\",\""); ESP.print(PASS); ESP.println("\"");
  String r = readESP(30000);
  DBG.print("Raspuns CWJAP: "); DBG.println(r);

  if (r.indexOf("WIFI GOT IP") != -1 || r.indexOf("OK") != -1) {
    DBG.println("✅ Conectat.");
    ESP.println("AT+CIFSR"); DBG.print("IP: "); DBG.println(readESP(1500));
  } else {
    explainError(parseCwJapError(r));
    DBG.println("❌ NU s-a conectat. Seteaza 2.4GHz, WPA2-PSK(AES), canal 1/6/11.");
  }
}
void loop() {}

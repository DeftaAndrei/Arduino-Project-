// Vectorul cu pinii de la 24 la 39 (relee 1-16)
const int relee[] = {
  24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39
};

// Starea curentă a fiecărui releu (0 = oprit, 1 = pornit)
bool stareReleu[16] = {0};

void setup() {
  Serial.begin(9600);
  Serial.println("Arduino Mega READY. Tasteaza 1-16 pentru a controla releele.");

  // Inițializare pini ca OUTPUT și setare relee oprite (HIGH = off, active LOW)
  for (int i = 0; i < 16; i++) {
    pinMode(relee[i], OUTPUT);
    digitalWrite(relee[i], HIGH);
  }
}

void loop() {
  if (Serial.available()) {
    String comanda = Serial.readStringUntil('\n');  // citește linia completă
    comanda.trim();  // elimină spații și newline
    
    // Verificăm dacă e un număr între 1 și 16
    int nr = comanda.toInt();
    if (nr >= 1 && nr <= 16) {
      int index = nr - 1;

      // Inversăm starea curentă
      stareReleu[index] = !stareReleu[index];
      digitalWrite(relee[index], stareReleu[index] ? LOW : HIGH);  // Active LOW
      
      Serial.print("Releu ");
      Serial.print(nr);
      Serial.println(stareReleu[index] ? " PORNIT" : " OPRIT");
    } else {
      Serial.println("Comanda invalida. Tasteaza un numar intre 1 si 16.");
    }
  }
}

#define BLYNK_TEMPLATE_ID "TMPL49bQPP5Y5"
#define BLYNK_TEMPLATE_NAME "Master Of Puppets"
#define BLYNK_DEVICE_NAME "ESP32_Device"
#define BLYNK_AUTH_TOKEN "cv-SMbQigyf7FO2JD2ky1uZ_FkckxWmR"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ArduinoJson.h> 

#define RX_PIN 16 
#define TX_PIN 17
#include <SoftwareSerial.h>
SoftwareSerial esp32Serial(RX_PIN, TX_PIN);

char ssid[] = "OnePlus Nord 3 5G";     
char pass[] = "x8ssryrh";

#define LED_PIN 2 

unsigned long previousMillis = 0;
const long interval = 100; // Tarkistetaan sarjadata useammin

unsigned long viimeinenPaivitys = 0;
const unsigned long paivitysvali = 12500;  // 12,5 sekuntia

int testiTunti = 0;
int testiRank = 0;

String viimeisinViesti = "";


// show ledit 
int showLed1 = 27; // Punaiset
int showLed2 = 26; // Keltaiset
int showLed3 = 25; // Vihreät
int showLed4 = 33; // Siniset

// Pinneille määritetyt buzzerit
#define BUZZPIN 32
// Äänitaajuuksia (Hz) – tärkeimmät nuotit
#define C4  262
#define E4  330
#define G4  392
#define C5  523
#define E5  659
#define G5  784
#define A4  440
#define B4  494
//intervallit
int triplet = 200/3;
int fullNote = 800;
int quaver = 800/4;
int semiQuaver = 800/8;
int sixteenth = 800/16;
int tripletHalf = 400/3;
int half = 800/2;


void setup() {
  Serial.begin(115200);
  esp32Serial.begin(9600);
  Serial.println("SoftwareSerial toimii!");

  pinMode(LED_PIN, OUTPUT);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Yhdistetään Wi-Fi-verkkoon...");
  }
  Serial.println("Wi-Fi-yhteys muodostettu");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  pinMode(showLed1, OUTPUT); // Asetetaan showLedien portti syöttämään virtaa ledeille
  pinMode(showLed2, OUTPUT); // Asetetaan showLedien portti syöttämään virtaa ledeille
  pinMode(showLed3, OUTPUT); // Asetetaan showLedien portti syöttämään virtaa ledeille
  pinMode(showLed4, OUTPUT); // Asetetaan showLedien portti syöttämään virtaa ledeille

  digitalWrite(showLed3, HIGH);

}

void paivitaAikaJaRank() {
  unsigned long nyt = millis();
  if (nyt - viimeinenPaivitys >= paivitysvali) {
    viimeinenPaivitys = nyt;

    testiTunti = (testiTunti + 1) % 24;
    testiRank = (testiRank + 1) % 24;

    char aikaBuffer[6];
    sprintf(aikaBuffer, "%02d:%02d", testiTunti, 0);
    viimeisinViesti = "TIME:" + String(aikaBuffer) + " RANK:" + String(testiRank);
  }
}

void tarkistaSarjadata() {
  while (esp32Serial.available()) {
    String pyynto = esp32Serial.readStringUntil('\n');
    pyynto.trim();
    Serial.println("Genuinolta: " + pyynto);

    if (pyynto == "GET_TIME") {
      paivitaAikaJaRank();  // Varmistetaan että aika on ajan tasalla
      Serial.println("Lähetetään: " + viimeisinViesti);
      esp32Serial.println(viimeisinViesti);
    
    }


    else {
      // Alustetaan JSON vastaanotto
      StaticJsonDocument<1024> vastaanotto;
      DeserializationError error = deserializeJson(vastaanotto, pyynto);

      if (error) {
        Serial.print("deserializeJson() epäonnistui: ");
        Serial.println(error.c_str());
        return; // Hypätään yli jos virhe
      }

      JsonObject mittaukset = vastaanotto.as<JsonObject>();

      // Taulukko: muuttujan nimi ja Blynk-virtuaalipinni
      struct DataMap {
        const char* key;
        int vPin;
      };

      DataMap mappings[] = {
        {"LAMPOTILA", V1},
        {"KOSTEUS", V2},
        {"VALOISUUS", V3},
        {"VALOTUNNIT", V4},
        {"VALONTARVETUNTEINA", V5},
        {"ARVOSANA", V6},
      };

      int mappingsCount = sizeof(mappings) / sizeof(mappings[0]);

      // Käydään kaikki määritellyt kentät läpi
      for (int i = 0; i < mappingsCount; i++) {
        const char* key = mappings[i].key;
        int vPin = mappings[i].vPin;

        // Tarkistetaan löytyykö avain
        if (mittaukset.containsKey(key)) {
          int arvo = mittaukset[key];
          // Erikoiskäsittely ECOMODE-napille: Button Widget tarvitsee 0 tai 1
          if (strcmp(key, "ECOMODE") == 0) {
            Blynk.virtualWrite(vPin, arvo ? 1 : 0);
          } else {
            Blynk.virtualWrite(vPin, arvo);
          }
        } 
        else {
          // Jos arvoa ei ole, lähetetään nolla
          if (strcmp(key, "ECOMODE") == 0) {
            Blynk.virtualWrite(vPin, 0); // Button pois päältä
          } else {
            Blynk.virtualWrite(vPin, 0);
          }
        }
      }
    }
  }
}



void loop() {
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    tarkistaSarjadata();
  }

  Blynk.run();
}

BLYNK_WRITE(V0) {
  int pinValue = param.asInt(); 
  digitalWrite(LED_PIN, pinValue);
}

BLYNK_WRITE(V7) {
  int pinValue = param.asInt(); 
  Serial.print("Nappi painettu, arvo: ");
  Serial.println(pinValue);
  if (pinValue == 255) {
    Serial.println("Soitetaan banger!");
    banger();
  }
}

void banger() {
  // Esimerkki: C-duurisointu
  for (int i = 0; i < 8; i++) {
    playTone(C4);
    
    delay(triplet);
    playTone(E4);
    digitalWrite(showLed1, LOW);
    digitalWrite(showLed2, HIGH);
    delay(triplet);
    playTone(G4);
    digitalWrite(showLed2, LOW);
    digitalWrite(showLed3, HIGH);
    delay(triplet);
  }

  

  for (int i = 0; i < 8; i++) {
    playTone(C4);
    
    delay(triplet);
    playTone(G4);
    digitalWrite(showLed4, LOW);
    digitalWrite(showLed1, HIGH);
    delay(triplet);
    playTone(B4);
    
    delay(triplet);
  }
  
  

  for (int i = 0; i < 8; i++) {
    playTone(C4);
    
    delay(triplet);
    playTone(E4);
    digitalWrite(showLed3, LOW);
    digitalWrite(showLed4, HIGH);
    delay(triplet);
    playTone(G4);
    
    delay(triplet);
  }
  
  

  for (int i = 0; i < 8; i++) {
    playTone(C4);
    
    delay(triplet);
    playTone(G4);
    digitalWrite(showLed2, LOW);
    digitalWrite(showLed3, HIGH);
    delay(triplet);
    playTone(B4);
   
    delay(triplet);
  }

  

  for (int i = 0; i < 8; i++) {
    playTone(B4);
    
    delay(triplet);
    playTone(G5);
    digitalWrite(showLed1, LOW);
    digitalWrite(showLed2, HIGH);
    delay(triplet);
    playTone(E5);
    
    delay(triplet);
  }

  

  playTone(G5);
  digitalWrite(showLed3, LOW);
  digitalWrite(showLed4, HIGH);
  delay(fullNote);

  for (int i = 0; i < 4; i++) {
    playTone(E4);
    
    delay(tripletHalf);
    playTone(G5);
    digitalWrite(showLed1, LOW);
    digitalWrite(showLed2, HIGH);
    delay(tripletHalf);
    playTone(B4);
    
    delay(tripletHalf);
  }

  for (int i = 0; i < 4; i++) {
    playTone(A4);
    
    delay(tripletHalf);
    playTone(G4);
    digitalWrite(showLed4, LOW);
    digitalWrite(showLed3, HIGH);
    delay(tripletHalf);
    playTone(C4);
    
    delay(tripletHalf);
  }

  for (int i = 0; i < 4; i++) {
    playTone(E4);
    
    delay(tripletHalf);
    playTone(G5);
    digitalWrite(showLed1, LOW);
    digitalWrite(showLed4, HIGH);
    delay(tripletHalf);
    playTone(B4);
    
    delay(tripletHalf);
  }

  for (int i = 0; i < 4; i++) {
    playTone(A4);
  
    delay(tripletHalf);
    playTone(G4);
    digitalWrite(showLed2, LOW);
    digitalWrite(showLed1, HIGH);
    delay(tripletHalf);
    playTone(C4);
    
    delay(tripletHalf);
  }

  for (int i = 0; i < 4; i++) {
    playTone(C5);
    
    delay(tripletHalf);
    playTone(G5);
    digitalWrite(showLed1, LOW);
    digitalWrite(showLed2, HIGH);
    delay(tripletHalf);
    playTone(E4);
    
    delay(tripletHalf);
  } 

  for (int i = 0; i < 4; i++) {
    playTone(G5);
    
    delay(tripletHalf);
    playTone(E5);
    digitalWrite(showLed4, LOW);
    digitalWrite(showLed3, HIGH);
    delay(tripletHalf);
    playTone(A4);
    
    delay(tripletHalf);
  }

  for (int i = 0; i < 4; i++) {
    playTone(A5);
    
    delay(tripletHalf);
    playTone(E5);
    digitalWrite(showLed1, LOW);
    digitalWrite(showLed2, HIGH);
    delay(tripletHalf);
    playTone(G4);
    
    delay(tripletHalf);
  }

    playTone(C4);
    
    delay(triplet);
    playTone(E4);
    digitalWrite(showLed4, LOW);
    digitalWrite(showLed1, HIGH);
    delay(triplet);
    playTone(G4);
    
    delay(triplet);
  
    playTone(C4);
    
    delay(triplet);
    playTone(G4);
    digitalWrite(showLed3, LOW);
    digitalWrite(showLed4, HIGH);
    delay(triplet);
    playTone(B4);
    
    delay(triplet);

    playTone(B4);
    
    delay(triplet);
    playTone(G5);
    digitalWrite(showLed2, LOW);
    digitalWrite(showLed3, HIGH);
    delay(triplet);
    playTone(E5);
    
    delay(triplet);

    playTone(G5);
    delay(half);
    digitalWrite(showLed1, HIGH);
    digitalWrite(showLed2, HIGH);
    digitalWrite(showLed3, HIGH);
    digitalWrite(showLed4, HIGH);
    playTone(1046);
    delay(half);
    digitalWrite(showLed1, LOW);
    digitalWrite(showLed2, LOW);
    digitalWrite(showLed3, LOW);
    digitalWrite(showLed4, LOW);
    noTone(BUZZPIN);

}

void playTone(int freq) {
  tone(BUZZPIN, freq); // sama pinni
}

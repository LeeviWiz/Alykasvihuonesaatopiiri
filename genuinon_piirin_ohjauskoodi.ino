// Kirjastojen tuonti
#include <DHT.h> // lisätään DHT-kirjaston peruskirjasto (funktiot lämpötila ja kosteuden lukemiseen)
#include <DHT_U.h> // lisätään DHT-kirjaston laajennettu versio (Adafruit Unified Sensor -järjestelmä mukana ja auttaa anturityyppien sekä muiden Adafruit antureiden saman aikaisen hallitsemisen)
#include <Servo.h> // lisätään Servo-kirjasto
#include <Wire.h> // I2C väylän kirjasto käytännössä
#include <LiquidCrystal_I2C.h> // Tuodaan I2C protokollalla keskustelevan LCD näytön kirjasto
#include <ArduinoJson.h> // Tuodaan Json-tiedostojen luomista varten kirjasto
 
// Koodissa käytettävien muuttujien luonti
const int DHTPIN = 10; // Määritellään DHTPIN muuttuja saamaan pelkkiä vakio kokonaislukuarvoja. Arvoksi tulee digitalport, johon kytkemme single-wire-protokollaa käyttäen DHT22 anturimme DATA-pinin.  
const int sensorPin = A0;  // Määritellään sensorPin muuttuja olemaan TEMT6000 signaalin tuloportti Analogpin A0 (Käytämme Arduino Genuino Zero)
const int releLamppuPin = 13; //Määritellään lämpölamppujen releen ohjaukselle digitalport 13
int servo_olosuhde_pin = 9; // Servo olosuhdemittarille
int servo_luukku_pin = 8; // Servo ilmaluukulle
int servo_kosteus_pin = 6;
const int ledEkoMode = 7;
String kasteluAjankohta = "07.00"; // Ajankohta, jolloin kasvi1 kastellaan.
const int releKastelu1 = 12;
const int releValoLamppu = 11;
 
float lampotila = 0.0;
float kosteus = 0.0;
int kosteuspisteet; // Määritellään kosteuspisteet kosteuden arviointia varten (pisteytysjärjestelmä optimaalinen - tyydyttävä - hälyttävä)
int lampotilapisteet; // Määritellään lämpötilapisteet lämpötilan arviointia varten (pisteytysjärjestelmä optimaalinen - tyydyttävä - hälyttävä)
int arvosana; // Määritellään muuttuja, johon varastoidaan kosteuspisteiden ja lämpötilapisteiden summa
float voltage; // Määritellään photoresistiivisen valoanturin referenssi jännite
float lux; // Määritellään valoanturin mittaamaat lux arvot tähän muuttujaan varastoitavaksi.
 
 
unsigned long viimeinenNayttoVaihto = 0;
bool PerusNakyma = true; // true = lämpö/kosteus/lux, false = valoisa aika
 
unsigned long viimeisinTuntiPaivitys = 0;
unsigned long viimeinenValoisuusPaivitys = 0;
int valoMinuutit = 0;
float valoTunnit = 0.0;
int valonTarveTunteina = 12;
float valoLamppujenTarve = 0;
int nykyinenTunti = 0;
int nykyinenMinuutti = 0;
bool nollattuTanaan = false;
int rank = -1; // monenneksi kallein tunti (0 = halvin, 23 = kallein)
String kello = "";
int viimeisinNollausTunti = -1;
unsigned long viimeisinAikaViesti = 0;
const unsigned long maksimiAikaviive = 12500;
int edellinenTunti = -1;
 
int kalliinSahkonLammitysraja = 17;
int normaaliLammitysraja = 19;
int nykyinenRaja = 0;
 
bool kasteltuTanaan = false;  // Merkitään, onko kastelu jo tehty
const int kasteluaikaSek = 5; // Kasteluaika sekunteina

// Kattoluukkujen ohjausta varten tarvittavat muuttujat
const float min_lampotila = 20.0; // Määritellään minimilämpötila, jolla luukku ei enää nouse
const float max_lampotila = 25.0; // Määritellään maksimilämpötila, jossa luukku on täysin maksimissaan auki
int servoKulma1; // Määritelläään muuttuja servonkulmalle, joka nostaa luukkua

// Kirjastojen luokkien olioiden luonti
Servo servoOlosuhde; // servo 1: mittari
Servo servoLuukku; // servo 2: ilmaluukku
Servo servoKosteus;
 
DHT dht(DHTPIN, DHT22); // Tämä luo olion dht, joka edustaa DHT22-anturia ja on yhdistetty pinnille DHTPIN.
LiquidCrystal_I2C lcd(0x27, 16, 2); // Määritellään olioksi lcd meidän LCD.näyttömme. Sen osoite 0x27 heksadesimaaleina ja sarakkeet 16 ja rivit 2.
 
// Koodin alustukset
void setup() {
 
  // Sarjamonitorin alustus
  Serial.begin(9600);
  delay(1000);
  Serial.println(">>> Setup alkaa!");
 
  Serial1.begin(9600);  // ESP8266:lta tuleva data
  delay(1000);
  Serial.println(">>> Serial1 aloitettu!");
 
  // DHT-anturin alustus
  dht.begin(); // alustetaan dht olion anturi mittaamaan
  delay(1000);
 
  // Servojen alustus
  servoOlosuhde.attach(servo_olosuhde_pin); // Kytke olosuhdemittarin servo
  servoLuukku.attach(servo_luukku_pin); // Kytke ilmaluukkujen servo
  servoKosteus.attach(servo_kosteus_pin); // Kytke kosteudenpoistoluulun servo
  // Releiden alustus
  pinMode(releLamppuPin, OUTPUT); // Lämpölamppujen ohjauksen alustus
  pinMode(releKastelu1, OUTPUT); // Kastelun ohjauksen alustus
  pinMode(releValoLamppu, OUTPUT);
  pinMode(ledEkoMode, OUTPUT);
 
  digitalWrite(releLamppuPin, HIGH);
  digitalWrite(releKastelu1, HIGH);
  digitalWrite(releValoLamppu, HIGH);
  digitalWrite(ledEkoMode, HIGH);
  delay(1000);
  digitalWrite(releLamppuPin, LOW);
  digitalWrite(releKastelu1, LOW);
  digitalWrite(releValoLamppu, LOW);
  digitalWrite(ledEkoMode, LOW);
  // LCD-näytön alustus
  lcd.init();
  delay(1000);
  lcd.backlight(); // Laitetaan LCD-näytön taustavalo päälle
 
  // Servojen testiliike (max–min)
  Serial.println("Servojen testaus");
  delay(1000);
  // Mittariservo (olosuhteet)
  servoOlosuhde.write(120);
  servoLuukku.write(120);
  servoKosteus.write(120);
  delay(1000);
 
  servoOlosuhde.write(0);
  servoLuukku.write(0);
  servoKosteus.write(0);
  delay(1000);
 
  Serial.println("Servot asetettu lähtöasentoon.");
  delay(1000);
}
 
// Simuloitu kello varalle, jos ESP ei lähetä, tai jostain syystä ei saada ulkopuolelta kellonaikaa
void paivitaSimuloituAika() {
  if (millis() - viimeisinTuntiPaivitys >= 12500) {
    nykyinenTunti++;
    if (nykyinenTunti >= 24) {
      nykyinenTunti = 0;
    }
    viimeisinTuntiPaivitys = millis();
 
    Serial.print("VARAKELLO: ");
    Serial.print(nykyinenTunti);
    Serial.println(":00");
  }
}


// Pistejärjestelmä olosuhdemittarille
void pisteytys() {
 
 
  // Pistejärjestelmä mitatulle kosteudelle
  if (kosteus >= 65 && kosteus <= 70) {
    kosteuspisteet = 60; // Jos kosteus on tältä väliltä
  } else if (kosteus >= 60 && kosteus <= 75) {
    kosteuspisteet = 45; // Jos kosteus ei ollut edelliseltä väliltä vaan tältä väliltä, annetaan arvioinnistamme pisteiksi kosteuspisteet = 20.
  } else if (kosteus >= 55 && kosteus <= 80) {
    kosteuspisteet = 30;
  } else if (kosteus >= 50 && kosteus <= 85) {
    kosteuspisteet = 15;
  } else {
    kosteuspisteet = 0;
  }
 
  // Tarkistetaan pisteet ja lämpötila-arvo
  Serial.println("Kosteus: " + String(kosteus) + " % " + "|| Pisteet: " + String(kosteuspisteet));
 
  // Pistejärjestelmä mitatulle lämpötilalle
  if (lampotila >= 19 && lampotila <= 22) {
    lampotilapisteet = 80;
  } else if (lampotila >= 18 && lampotila <= 23) {
    lampotilapisteet = 60;
  } else if (lampotila >= 17 && lampotila <= 24) {
    lampotilapisteet = 40;
  } else if (lampotila >= 16 && lampotila <= 25) {
    lampotilapisteet = 20;
  } else {
    lampotilapisteet = 0;
  }
 
  // Tarkistetaan pisteet ja lämpötila-arvo
  Serial.println("Lämpötila: " + String(lampotila) + " °C " + "|| Pisteet: " + String(lampotilapisteet));
 
  // Tarkistetaan pistejärjestelmän arvosana
  arvosana = kosteuspisteet + lampotilapisteet;
  Serial.println("Arvosana: " + String(arvosana));
}
 
// LCD näytön operointi
void paivitaNaytto() {
 
  lcd.setCursor(0, 0);
  lcd.print("                "); // tyhjennä rivit manuaalisesti
  lcd.setCursor(0, 1);
  lcd.print("                ");
 
  lcd.setCursor(0, 0);
  lcd.print("L:");
  lcd.print(lampotila, 1);
  lcd.print((char)223); lcd.print("C ");
 
  lcd.setCursor(9, 0);
  lcd.print("K:");
  lcd.print(kosteus, 1);
  lcd.print("%");
 
  lcd.setCursor(0, 1);
  lcd.print("Valotunnit ");
  lcd.print(valoTunnit, 2);
}
 
// Ilmaluukkujen ohjaus lämpötilan perusteella
void ohjaaIlmaluukkujaLampotilanPerusteella(float lampotila) {
  
 
  if (lampotila <= min_lampotila) { // Lämpötilan ollessa pienempi tai yhtä suuri kuin minimilämpötila, luukut on kokonaan suljettuna
    servoKulma1 = 0;
  } else if (lampotila >= max_lampotila) { // Lämpötilan ollessa suurempi tai yhtä suuri kuin maksimilämpötila, luukut kokonaan auki
    servoKulma1 = 90;
  } else {
    servoKulma1 = map(lampotila * 10, min_lampotila * 10, max_lampotila * 10, 0, 90); // Minimi- ja maksimiarvojen välillä luukut avautuvat pikkuhiljaa lämpötilan mukaan
  }
 
  static int edellinenKulma = -1;
  if (abs(servoKulma1 - edellinenKulma) >= 3) {
    servoLuukku.write(servoKulma1);
    edellinenKulma = servoKulma1;
 
    Serial.println("Ilmaluukut säädetty:");
    Serial.print("  Lämpötila: ");
    Serial.print(lampotila);
    Serial.print(" °C => Servo: ");
    Serial.print(servoKulma1);
    Serial.println("°");
  }
 
  // Tulostetaan asento serial monitoriin joka loopilla, vaikka ei muuttuisi
  Serial.println("Ilmaluukun servon nykyinen asento: " + String(servoKulma1) + "°");
}
 
// Lämpölamppujen releohjaus
void ohjaaLampotilaaLampuilla(float lampotila) {
  const float kriittinenRaja = nykyinenRaja; // Määritellään minimilämpötila jolloin lamput syttyvät
  const float palautusraja = nykyinenRaja + 1.0; // Määritellään palautuslämpötila jolloin lamput sammuvat
 
  static bool lamppuPaalla = false;
 
  if (lampotila < kriittinenRaja && !lamppuPaalla) { // Kun lämpötila on pienempi kuin minimilämpötila, lamput päälle
    digitalWrite(releLamppuPin, HIGH); // Lamput päälle
    lamppuPaalla = true;
    Serial.println("Lämpölamppu kytketty PÄÄLLE");
  }
  else if (lampotila > palautusraja && lamppuPaalla) { // Kun lämpötila ylittää palautuslämpötilan, lamput pois
    digitalWrite(releLamppuPin, LOW); // Lamput pois
    lamppuPaalla = false;
    Serial.println("Lämpölamppu kytketty POIS PÄÄLTÄ");
  }
}
 
// Kastelujärjestelmän ohjaus, kastelu kerran vuorokaudessa kello 07:00
void kastelu() {
  if (nykyinenTunti == 7 && nykyinenMinuutti == 0 && !kasteltuTanaan) {
    Serial.println("Kastelu käynnissä...");
 
    // Kastellaan kasvi 1
    digitalWrite(releKastelu1, HIGH);
    delay(kasteluaikaSek * 1000);
    digitalWrite(releKastelu1, LOW);
    Serial.println("Kasvi 1 kasteltu");
 
    kasteltuTanaan = true;
  }

  else {

    Serial.println("Seuraava kastelu klo: " + String(kasteluAjankohta));

  }
}
 
// Valotuntilaskuri joka laskee tunnit jolloin mitattu lux-määrä ylittää raja-arvon
void laskeValoisaaAikaa() {
  if (nykyinenTunti != edellinenTunti) {
    edellinenTunti = nykyinenTunti;
    
    bool lisataanValoa = false;
 
    if(lux > 175) {
      lisataanValoa = true;
    } else if (digitalRead(releValoLamppu) == HIGH) {
      lisataanValoa = true;
  }
 
  if (lisataanValoa) {
    valoMinuutit += 60;
    valoTunnit = valoMinuutit / 60.0;
    }
  }
}
 
// Lämpölamppujen ohjaus niin, että kalliin sähkön aikaan ei lämmitä yhtä herkästi
void lamppujenOhjausHinnanPerusteella() {
  if (rank >= 11 && rank <= 23){
    nykyinenRaja = kalliinSahkonLammitysraja;
    } else {
      nykyinenRaja = normaaliLammitysraja; }
}
 
// Kellon ja sähkön hinnan tiedustelu ja vastaanotto ESP:ltä
void kellonJaRankinTiedustelu() {
  Serial1.println("GET_TIME");  // Pyyntö ESP:lle

  delay(100);  // Odotetaan hetki vastaukselle

  // Tarkistetaan saapuuko ESP:ltä dataa Serial1:n kautta
  while (Serial1.available()) {
    String rivi = Serial1.readStringUntil('\n');
    rivi.trim();

    if (rivi.startsWith("TIME:")) {
      kello = rivi.substring(5, 10);
      nykyinenTunti = rivi.substring(5, 7).toInt();
      nykyinenMinuutti = rivi.substring(8, 10).toInt();

      int rankIndex = rivi.indexOf("RANK:");
      if (rankIndex != -1) {
        rank = rivi.substring(rankIndex + 5).toInt();
      }

      Serial.println("SAATU SARJAVIESTI:");
      Serial.println(rivi);
    } else {
      Serial.print("Tuntematon viesti: ");
      Serial.println(rivi);
    }
  }

  // Luo JSON-dokumentti
  StaticJsonDocument<1024> mittaukset;  // Kasvatin vähän kokoa, 200 olisi aika tiukka näin isolle datalle

  // Täytetään mittaukset-dokumentti
  mittaukset["LAMPOTILA"] = lampotila;
  mittaukset["KOSTEUS"] = kosteus;
  mittaukset["VALOISUUS"] = lux;
  mittaukset["VALOTUNNIT"] = valoTunnit;
  mittaukset["VALONTARVETUNTEINA"] = valonTarveTunteina;
  mittaukset["VALOLAMPPUJENTARVE"] = valoLamppujenTarve;
  mittaukset["KASTELLAAN"] = kasteluAjankohta;
  mittaukset["KASTELTU"] = kasteltuTanaan;
  mittaukset["KASTELUAIKA"] = kasteluaikaSek;

  // Kello erikseen oliona
  JsonObject kello = mittaukset.createNestedObject("KELLO");
  kello["TUNTI"] = nykyinenTunti;
  kello["MINUUTTI"] = nykyinenMinuutti;

  mittaukset["KALLIINSÄHKÖNLÄMMITYSRAJA"] = kalliinSahkonLammitysraja;
  mittaukset["NORMAALILÄMMITYSRAJA"] = normaaliLammitysraja;
  mittaukset["NYKYINENRAJA"] = nykyinenRaja;
  mittaukset["MONENNEKSIKALLEINTUNTI"] = rank;
  mittaukset["ECOMODE"] = (rank >= 11 && rank <= 23) ? "ON" : "OFF";
  mittaukset["LAMPOTILAPISTEET"] = lampotilapisteet;
  mittaukset["KOSTEUSPISTEET"] = kosteuspisteet;
  mittaukset["ARVOSANA"] = arvosana;
  mittaukset["MINLÄMPÖTILA"] = min_lampotila;
  mittaukset["MAXLÄMPÖTILA"] = max_lampotila;
  mittaukset["servoKulma1"] = servoKulma1;

  // Lähetä JSON ulos sarjaporttiin
  serializeJson(mittaukset, Serial1);
  
  Serial.println("Lähetetään ESP:lle!");
}

 
// Valolamppujen releohjaus. Jos valoisat tunnit jäävät vajaaksi, valolamput korvaavat auringon laskiessa puuttuvan valomäärän
void valoLamppujenOhjaus() {
  valoLamppujenTarve = valonTarveTunteina - valoTunnit;
  if (valoLamppujenTarve > 0.05 && lux <= 3000 && nykyinenTunti >= 12) {
    digitalWrite(releValoLamppu, HIGH);
    Serial.println("Valolamppu PÄÄLLÄ - Valoisaa aikaa ei vielä riittävästi");
    } else {
      digitalWrite(releValoLamppu, LOW);
      Serial.println("Valolamppu POIS - valoisa aika täynnä");
      }
}
 
// Ekomoodi, ilmoittaa kun sähkö on kallista (vuorokauden 12 kalleinta tuntia)
void ekoMoodi() {
  if  (rank >= 11 && rank <= 23){
    digitalWrite(ledEkoMode, HIGH);
    Serial.println("!ECOMODE!SÄHKÖ KALLISTA!");
   } else {
    digitalWrite(ledEkoMode, LOW);
    }
}
 
// Kosteudenpoisto ilmaluukun avulla, kun mitattu kosteus ylittää raja-arvon
void kosteudenPoisto() {
  if (kosteus > 70) {
    int servoKulma2 = map(kosteus, 70, 85, 0, 90);
    servoKulma2 = constrain(servoKulma2, 0, 90);
    servoKosteus.write(servoKulma2);
    Serial.println("Kosteudenpoisto aktiivinen! ");
    Serial.print("Kosteudenpoistoluukku auki: ");
    Serial.print(servoKulma2);
    Serial.println("°");
    }
 
}
 
// Koodi
void loop() {
 
  kellonJaRankinTiedustelu();
 
  // Tarkista, onko nyt klo 00:00 ja onko nollattu tänään
  if (nykyinenTunti == 0 && nykyinenMinuutti == 0 && !nollattuTanaan) {
  valoMinuutit = 0;
  valoTunnit = 0.0;
  nollattuTanaan = true;
  Serial.println("Valoisa aika NOLLATTU klo 00:00");
  }
 
  // Jos aika ei enää ole 00:00, sallitaan seuraavan yön nollaus
  if (!(nykyinenTunti == 0 && nykyinenMinuutti == 0)) {
    nollattuTanaan = false;
  }
 
 
   // Määritellään kosteus ja lämpötila
  kosteus = dht.readHumidity();
  lampotila = dht.readTemperature();
 
  // Tarkistetaan palauttaako dht numeerisia arvoja mittauksille (jos kaikki ok) vai jotain huuhaata
  if (isnan(kosteus) || isnan(lampotila)) {
    Serial.println("Virhe: Ei saatu tietoa DHT-sensorilta!");
    return;
    }
 
  pisteytys();
 
  // Saadulla arvosanalla määritellään servon servomoottorin kulma servoKulma
   int servoKulma = map(arvosana, 0, 140, 0, 120);
  servoKulma = constrain(servoKulma, 0, 120);
  servoOlosuhde.write(servoKulma);
  Serial.println("Mittariservo (pisteet): " + String(servoKulma) + "°");
 
 
  // Määritellään valosensorin mittaamat luksit
  int rawValue = analogRead(sensorPin);  // Lue sensorin arvo (0-1023)  
  voltage = rawValue * (3.3 / 1023.0);  // Muunnetaan jännitteeksi (Zero käyttää 3.3V referenssiä)
  // Arvioidaan lux-arvo (TEMT6000 likimääräinen herkkyys ~10µA per 1 lux)
  lux = voltage * (1000.0 / 0.5);  // 1000 kohm vastuksen mukaan (oletuksena 2V ≈ 1000 lux) 1000/0.5 on sensorin arvioidun herkkyyden vakio
  Serial.println("Lux: " + String(lux));
 
  if (!(nykyinenTunti == 7 && nykyinenMinuutti == 0)) {
  kasteltuTanaan = false;}
 
  // Kutsu näytön operointia
  paivitaNaytto();
 
  // Kutsu valoisan ajan mittaria
  laskeValoisaaAikaa();
 
  // Kutsu kastelua
  kastelu();
 
  // Kutsu ilmaluukkujen ohjausta
  ohjaaIlmaluukkujaLampotilanPerusteella(lampotila);
 
  // Kutsu lämpölamppujen ohjausta
  ohjaaLampotilaaLampuilla(lampotila);
 
  // Lämpölamppujen ohjaus hinnan perusteella
  lamppujenOhjausHinnanPerusteella();
 
  // Valolamppujen ohjaus
  valoLamppujenOhjaus();
 
  ekoMoodi();
 
  kosteudenPoisto();
 
  if (millis() - viimeisinAikaViesti > maksimiAikaviive) {
  paivitaSimuloituAika();
  }
 
 
  delay(10000);
}


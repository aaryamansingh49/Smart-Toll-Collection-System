#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPL6qefrMFka"
#define BLYNK_TEMPLATE_NAME "Smart Gate Barrier System"
#define BLYNK_AUTH_TOKEN "DwKEsUtMnl29Tc7RnoPrTiRNlX7YC68j"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

char ssid[] = "Ankush";
char pass[] = "12345678";

// RFID
#define SS_PIN D4
#define RST_PIN D3
MFRC522 rfid(SS_PIN, RST_PIN);

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo
Servo gate;
#define SERVO_PIN D8

// Pins
#define GREEN_LED 3
#define RED_BUZZ  D0

int toll = 35;

// 🔥 VARIABLES
int totalRevenue = 0;
int carCount = 0;

// Cards
struct Card {
  String uid;
  int balance;
};

Card cards[] = {
  {"61 3B 3D 20", 2000},
  {"A1 A1 3D 20", 2000},
  {"7C E4 EF 06", 30}
};

// 🔊 Beep
void beepShort() {
  digitalWrite(RED_BUZZ, LOW);
  delay(200);
  digitalWrite(RED_BUZZ, HIGH);
}

void beepLong() {
  digitalWrite(RED_BUZZ, LOW);
  delay(1200);
  digitalWrite(RED_BUZZ, HIGH);
}

void setup() {
  Serial.begin(9600);

  Wire.begin(D2, D1);

  SPI.begin();
  rfid.PCD_Init();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_BUZZ, OUTPUT);

  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_BUZZ, HIGH);

  gate.attach(SERVO_PIN);
  gate.write(0);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("Smart Toll Sys");
  lcd.setCursor(0,1);
  lcd.print("Initializing...");
  delay(2000);

  lcd.clear();
  lcd.print("Scan Card...");
}

void loop() {
  Blynk.run();

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i != rfid.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();

  Serial.println("\n------ CARD SCANNED ------");
  Serial.print("UID: ");
  Serial.println(uid);

  lcd.clear();

  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_BUZZ, HIGH);

  bool found = false;

  for (int i = 0; i < 3; i++) {

    if (uid == cards[i].uid) {
      found = true;

      if (cards[i].balance >= toll) {

        cards[i].balance -= toll;

        // 🔥 UPDATE VALUES
        totalRevenue += toll;
        carCount++;

        // LCD
        lcd.setCursor(0,0);
        lcd.print("Paid Rs:35");
        lcd.setCursor(0,1);
        lcd.print("Bal:" + String(cards[i].balance));

        // Serial
        Serial.println("Access Granted");
        Serial.println("Paid Rs: 35");
        Serial.print("Remaining Balance: ");
        Serial.println(cards[i].balance);

        beepShort();

        digitalWrite(GREEN_LED, LOW);
        gate.write(90);

        // 🔥 BLYNK UPDATE (FINAL)
        Blynk.virtualWrite(V0, cards[i].balance);
        Blynk.virtualWrite(V1, uid);
        Blynk.virtualWrite(V2, "OPEN");
        Blynk.virtualWrite(V3, carCount);       // 🚗 Car Count
        Blynk.virtualWrite(V4, totalRevenue);   // 💰 Revenue

        // 🔥 EVENTS
        Blynk.logEvent("toll_paid",
          "Rs35 deducted | Bal: " + String(cards[i].balance)
        );

        Blynk.logEvent("gate_open",
          "Gate Open | UID: " + uid +
          " | Cars: " + String(carCount) +
          " | Revenue: " + String(totalRevenue)
        );

        delay(3000);

        gate.write(0);
        digitalWrite(GREEN_LED, HIGH);

      } else {

        lcd.setCursor(0,0);
        lcd.print("Low Balance");
        lcd.setCursor(0,1);
        lcd.print("Need Rs:35");

        Serial.println("Access Denied - Low Balance");
        Serial.print("Balance: ");
        Serial.println(cards[i].balance);

        beepLong();

        Blynk.logEvent("low_balance",
          "Bal low: " + String(cards[i].balance)
        );
      }

      break;
    }
  }

  if (!found) {
    lcd.print("Access Denied");
    Serial.println("Unknown Card!");
    beepLong();
  }

  Serial.println("--------------------------");

  delay(2000);

  lcd.clear();
  lcd.print("Scan Card...");
}
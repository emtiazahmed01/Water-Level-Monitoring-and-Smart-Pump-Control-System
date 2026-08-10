/*
   Water level monitoring system - ESP32 + Blynk IoT
   Validated in Wokwi simulation prior to hardware deployment
*/

// ---- Blynk credentials: complete these before building for hardware ----
#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Water level monitoring system"
#define BLYNK_PRINT Serial

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#define LED1 19
#define LED2 18
#define LED3 5
#define LED4 15

#define trig 12
#define echo 13
#define relay 14

#define SDA_PIN 21
#define SCL_PIN 22

#define LCD_REFRESH_SECONDS 60

int MaxLevel = 13;

// Thresholds are expressed as a percentage of WATER DEPTH, not of the air gap
int Level1 = (MaxLevel * 25) / 100;   // 25 % full
int Level2 = (MaxLevel * 50) / 100;   // 50 % full
int Level3 = (MaxLevel * 75) / 100;   // 75 % full
int Level4 = (MaxLevel * 90) / 100;   // 90 % full - triggers automatic shut-off

uint8_t lcdAddress = 0x27;
LiquidCrystal_I2C lcd(0x27, 16, 2);

BlynkTimer timer;

char auth[] = "YOUR_BLYNK_AUTH_TOKEN";

// Wokwi virtual network; substitute the real SSID and password for hardware
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

char lineTop[17]    = "                ";
char lineBottom[17] = "                ";

unsigned long lastRefresh = 0;
bool lcdReady = false;

// ---------------------------------------------------------------- LCD helpers

bool i2cPresent(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

uint8_t findLcd() {
  if (i2cPresent(0x27)) return 0x27;
  if (i2cPresent(0x3F)) return 0x3F;
  for (uint8_t a = 0x20; a <= 0x27; a++) if (i2cPresent(a)) return a;
  for (uint8_t a = 0x38; a <= 0x3F; a++) if (i2cPresent(a)) return a;
  return 0;
}

void setLine(char *buf, const char *text) {
  int i = 0;
  while (text[i] != '\0' && i < 16) { buf[i] = text[i]; i++; }
  while (i < 16) { buf[i] = ' '; i++; }
  buf[16] = '\0';
}

void drawLcd() {
  if (!lcdReady) return;
  lcd.setCursor(0, 0);
  lcd.print(lineTop);
  lcd.setCursor(0, 1);
  lcd.print(lineBottom);
}

void initLcd() {
  lcd.init();
  delay(60);
  lcd.backlight();
  lcd.clear();
  delay(10);
  lcdReady = true;
  drawLcd();
  lastRefresh = millis();
}

void serviceLcd() {
  if (!i2cPresent(lcdAddress)) {
    lcdReady = false;
    Serial.println("LCD not responding on I2C");
    uint8_t found = findLcd();
    if (found) { lcdAddress = found; lcd = LiquidCrystal_I2C(found, 16, 2); initLcd(); }
    return;
  }
  if (!lcdReady) { initLcd(); return; }

  if (LCD_REFRESH_SECONDS > 0 &&
      millis() - lastRefresh > (unsigned long)LCD_REFRESH_SECONDS * 1000UL) {
    initLcd();
  }
}

// ---------------------------------------------------------------- setup

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);
  digitalWrite(trig, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(50000);

  uint8_t found = findLcd();
  if (found == 0) {
    Serial.println("No I2C device found. Check SDA=21, SCL=22, VCC on VIN (5V), GND.");
  } else {
    Serial.print("LCD found at 0x");
    Serial.println(found, HEX);
    lcdAddress = found;
    lcd = LiquidCrystal_I2C(found, 16, 2);
    initLcd();
  }

  setLine(lineTop,    "System");
  setLine(lineBottom, "    Loading..");
  drawLcd();

  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);

  setLine(lineTop,    "");
  setLine(lineBottom, "Motor is OFF");
  drawLcd();

  timer.setInterval(1000L, ultrasonic);
}

// ---------------------------------------------------------------- sensor

void ultrasonic() {
  serviceLcd();

  digitalWrite(trig, LOW);
  delayMicroseconds(4);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long t = pulseIn(echo, HIGH, 30000UL);
  if (t == 0) {
    Serial.println("No echo - check sensor wiring");
    return;
  }

  int distance = t / 29 / 2;
  Serial.println(distance);

  // blynkDistance is the actual depth of the stored water
  int blynkDistance = MaxLevel - distance;
  if (blynkDistance < 0) blynkDistance = 0;
  Blynk.virtualWrite(V0, blynkDistance);

  char row[17];

  // Evaluated from the fullest state downwards
  if (blynkDistance >= Level4) {
    strcpy(row, "WLevel:Full");
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, HIGH);

    // --- automatic pump shut-off ---
    digitalWrite(relay, HIGH);             // de-energise the relay
    setLine(lineBottom, "Motor is OFF");   // reflect the change locally
    Blynk.virtualWrite(V1, 0);             // resynchronise the dashboard switch

  } else if (blynkDistance >= Level3) {
    strcpy(row, "WLevel:Medium");
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, LOW);
  } else if (blynkDistance >= Level2) {
    strcpy(row, "WLevel:Low");
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, LOW);
  } else if (blynkDistance >= Level1) {
    strcpy(row, "WLevel:Very Low");
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, LOW);
  } else {
    strcpy(row, "WLevel:Empty");
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, LOW);
  }

  setLine(lineTop, row);
  drawLcd();
}

// ---------------------------------------------------------------- Blynk

BLYNK_WRITE(V1) {
  bool Relay = param.asInt();
  if (Relay == 1) {
    digitalWrite(relay, LOW);
    setLine(lineBottom, "Motor is ON");
  } else {
    digitalWrite(relay, HIGH);
    setLine(lineBottom, "Motor is OFF");
  }
  drawLcd();
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V1);
}

void loop() {
  Blynk.run();
  timer.run();
}

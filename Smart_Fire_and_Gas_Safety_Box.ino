#define BLYNK_TEMPLATE_ID "from BLYNK app"
#define BLYNK_TEMPLATE_NAME "from BLYNK app"
#define BLYNK_AUTH_TOKEN "from BLYNK app"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "wifi_name";
char pass[] = "password";

#define DHTPIN 14
#define DHTTYPE DHT11

const int flamePin = 34;
const int gasPin = 35;
const int ledPin = 2;
const int buzzerPin = 27;
const int servoPin = 18;
const int motorPin = 26;

bool notified = false;
bool alarmMuted = false;
bool flameDetected = false;

DHT dht(DHTPIN, DHTTYPE);
Servo gasServo;
LiquidCrystal_PCF8574 lcd(0x27);

BLYNK_WRITE(V5) {
  alarmMuted = param.asInt();
}

void setup() {
  Serial.begin(115200);

  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);

  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);

  Wire.begin(21, 22);
  lcd.begin(16, 2);
  lcd.setBacklight(255);

  dht.begin();
  Blynk.begin(auth, ssid, pass);

  gasServo.attach(servoPin);
  gasServo.write(0);
}

void loop() {
  Blynk.run();

  int flameValue = analogRead(flamePin);
  int gasValue = analogRead(gasPin);
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp)) temp = 0;
  if (isnan(hum)) hum = 0;

  if (!flameDetected && flameValue < 1200) {
    flameDetected = true;
  } 
  else if (flameDetected && flameValue > 2500) {
    flameDetected = false;
  }

  bool gasDetected = gasValue > 3000;
  bool tempDetected = temp > 50;

  bool danger = flameDetected || gasDetected || tempDetected;

  if (gasDetected) {
    gasServo.write(90);
  } else {
    gasServo.write(0);
  }

  if (flameDetected) {
    digitalWrite(motorPin, HIGH);
  } else {
    digitalWrite(motorPin, LOW);
  }

  digitalWrite(ledPin, danger ? HIGH : LOW);

  if (danger && !alarmMuted) {
    digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(buzzerPin, LOW);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp);
  lcd.print("C");

  lcd.setCursor(8, 0);
  lcd.print("H:");
  lcd.print(hum);
  lcd.print("%");

  lcd.setCursor(0, 1);
  if (danger) {
    lcd.print("ALERT!");
  } else {
    lcd.print("SAFE   ");
  }

  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, hum);
  Blynk.virtualWrite(V2, flameDetected);
  Blynk.virtualWrite(V3, gasDetected);
  Blynk.virtualWrite(V4, danger);

  if (danger && !notified) {
    Blynk.logEvent("danger_alert", "Fire or Gas Detected!");
    notified = true;
  }

  if (!danger) {
    notified = false;
  }

  if (!danger && alarmMuted) {
    alarmMuted = false;
    Blynk.virtualWrite(V5, 0);
  }

  delay(200);
}

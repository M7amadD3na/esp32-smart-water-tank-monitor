#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

// ============================================================
//   إعدادات الشبكة و ThingSpeak
// ============================================================
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
String      apiKey   = "YOUR_THINGSPEAK_WRITE_API_KEY";
const char* host     = "api.thingspeak.com";

// ============================================================
//   تعريف الـ Pins
// ============================================================
const int trigPin   = 14;
const int echoPin   = 25;
const int redLed    = 26;
const int yellowLed = 27;
const int greenLed  = 33;
const int buzzer    = 32;

// ============================================================
//   معايرة المسافات
// ============================================================
const int DIST_FULL  = 15;
const int DIST_EMPTY = 80;

// ============================================================
//   متغيرات عامة
// ============================================================
WebServer  server(80);
WiFiClient client;

int    waterPercentage = 0;
String tankStatus      = "Initializing";
String alertMessage    = "System Online";
String lastRefillTime  = "N/A";
bool   refillFlag      = false;

// ============================================================
//   إعلان الدوال
// ============================================================
void updateLogic(int percent);
void sendToThingSpeak(int level);
void handleRoot();
String getBarColor(int percent);

// ============================================================
//   setup
// ============================================================
void setup() {
  Serial.begin(115200);

  pinMode(trigPin,   OUTPUT);
  pinMode(echoPin,   INPUT);
  pinMode(redLed,    OUTPUT);
  pinMode(yellowLed, OUTPUT);
  pinMode(greenLed,  OUTPUT);
  pinMode(buzzer,    OUTPUT);

  digitalWrite(redLed,    LOW);
  digitalWrite(yellowLed, LOW);
  digitalWrite(greenLed,  LOW);
  noTone(buzzer);

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 30) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection FAILED. Running offline.");
  }

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started.");
}

// ============================================================
//   loop
// ============================================================
void loop() {
  server.handleClient();

  static unsigned long lastRead = 0;
  if (millis() - lastRead < 500) return;
  lastRead = millis();

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);

  if (duration == 0) {
    waterPercentage = 0;
    tankStatus      = "Sensor Error";
    alertMessage    = "Check Sensor Wiring";
    Serial.println("[ERROR] No echo received from sensor.");
  } else {
    float distance = (duration * 0.0343f) / 2.0f;

    Serial.print("[INFO] Distance: ");
    Serial.print(distance);
    Serial.print(" cm | ");

    int rawPercent = map((int)distance, DIST_EMPTY, DIST_FULL, 0, 100);
    waterPercentage = constrain(rawPercent, 0, 100);

    Serial.print("Level: ");
    Serial.print(waterPercentage);
    Serial.println("%");

    updateLogic(waterPercentage);
  }

  static unsigned long lastUpload = 0;
  if (millis() - lastUpload > 20000) {
    if (WiFi.status() == WL_CONNECTED) {
      sendToThingSpeak(waterPercentage);
    } else {
      Serial.println("[WARN] WiFi disconnected, skipping upload.");
    }
    lastUpload = millis();
  }
}

// ============================================================
//   updateLogic
// ============================================================
void updateLogic(int percent) {
  digitalWrite(greenLed,  LOW);
  digitalWrite(yellowLed, LOW);
  digitalWrite(redLed,    LOW);
  noTone(buzzer);

  if (percent >= 98) {
    digitalWrite(greenLed, HIGH);
    tone(buzzer, 2000, 500);
    tankStatus   = "Overflow!";
    alertMessage = "DANGER: Tank Overflowing!";
    if (refillFlag) { lastRefillTime = "Just Now"; refillFlag = false; }
  }
  else if (percent >= 75) {
    digitalWrite(greenLed, HIGH);
    tankStatus   = "Full";
    alertMessage = "Tank is Full";
    if (refillFlag) { lastRefillTime = "Just Now"; refillFlag = false; }
  }
  else if (percent > 25) {
    digitalWrite(yellowLed, HIGH);
    tankStatus   = "Medium";
    alertMessage = "Water Level Normal";
  }
  else {
    digitalWrite(redLed, HIGH);
    tone(buzzer, 1000, 200);
    tankStatus   = "Low";
    alertMessage = "CRITICAL: Refill Now!";
    refillFlag   = true;
  }
}

// ============================================================
//   sendToThingSpeak
// ============================================================
void sendToThingSpeak(int level) {
  Serial.print("[INFO] Sending to ThingSpeak: ");
  Serial.println(level);

  if (client.connect(host, 80)) {
    String url = "/update?api_key=" + apiKey + "&field1=" + String(level);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 3000) {
        Serial.println("[WARN] ThingSpeak timeout.");
        break;
      }
    }

    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }

    client.stop();
    Serial.println("\n[INFO] ThingSpeak upload done.");
  } else {
    Serial.println("[ERROR] Could not connect to ThingSpeak.");
  }
}

// ============================================================
//   getBarColor
// ============================================================
String getBarColor(int percent) {
  if (percent >= 75) return "#22c55e";
  if (percent > 25)  return "#f59e0b";
  return "#ef4444";
}

// ============================================================
//   handleRoot
// ============================================================
void handleRoot() {
  String color = getBarColor(waterPercentage);

  String s = "<!DOCTYPE html><html><head>";
  s += "<meta charset='UTF-8'>";
  s += "<meta http-equiv='refresh' content='3'>";
  s += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  s += "<title>Smart Tank Monitor</title>";
  s += "<style>";
  s += "* { box-sizing: border-box; margin: 0; padding: 0; }";
  s += "body { font-family: Arial, sans-serif; background: #0f172a; color: #e2e8f0;";
  s += "       display: flex; justify-content: center; align-items: center; min-height: 100vh; }";
  s += ".card { background: #1e293b; border-radius: 20px; padding: 40px 35px;";
  s += "        width: 340px; box-shadow: 0 20px 60px rgba(0,0,0,0.5);";
  s += "        border: 1px solid #334155; text-align: center; }";
  s += "h1 { font-size: 1.2rem; color: #38bdf8; letter-spacing: 3px; margin-bottom: 30px; }";
  s += ".percent { font-size: 3.5rem; font-weight: 700; color:" + color + "; margin: 10px 0; }";
  s += ".bar-bg { background: #334155; border-radius: 10px; height: 20px; overflow: hidden; margin: 20px 0; }";
  s += ".bar-fill { height: 100%; width:" + String(waterPercentage) + "%;";
  s += "            background:" + color + "; border-radius: 10px; }";
  s += ".info-row { display: flex; justify-content: space-between; padding: 10px 0;";
  s += "            border-bottom: 1px solid #334155; font-size: 0.9rem; }";
  s += ".info-row:last-of-type { border-bottom: none; }";
  s += ".label { color: #94a3b8; }";
  s += ".value { font-weight: bold; }";
  s += ".alert-box { margin-top: 20px; padding: 12px; border-radius: 10px;";
  s += "             background: #0f172a; border: 1px solid #475569; font-size: 0.85rem; color: #f1f5f9; }";
  s += ".footer { margin-top: 25px; font-size: 0.75rem; color: #475569; }";
  s += "</style></head><body><div class='card'>";
  s += "<h1>SMART TANK MONITOR</h1>";
  s += "<div class='percent'>" + String(waterPercentage) + "%</div>";
  s += "<div class='bar-bg'><div class='bar-fill'></div></div>";
  s += "<div class='info-row'><span class='label'>Status</span><span class='value'>" + tankStatus + "</span></div>";
  s += "<div class='info-row'><span class='label'>Alert</span><span class='value'>" + alertMessage + "</span></div>";
  s += "<div class='info-row'><span class='label'>Last Refill</span><span class='value'>" + lastRefillTime + "</span></div>";
  s += "<div class='alert-box'>" + alertMessage + "</div>";
  s += "<div class='footer'>Auto-refresh every 3s | ThingSpeak sync every 20s</div>";
  s += "</div></body></html>";

  server.send(200, "text/html", s);
}
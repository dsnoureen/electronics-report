#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <RTClib.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// ===== CONFIG =====
const char *ssid = "ewaste";
const char *password = "ewaste1234";

#define LED_PIN 2      // Onboard LED D4/GPIO2
#define LCD_ADDR 0x27  // Adjust if needed
#define LCD_COLS 16
#define LCD_ROWS 2

const unsigned long FETCH_INTERVAL_MS = 10UL * 1000UL;  // 10 seconds
const int BEFORE_MIN = 15;                              // minutes before meal
const int AFTER_MIN = 15;                               // minutes after meal
const int TIMEZONE_OFFSET = 6;                          // UTC+6

// ===== OBJECTS =====
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
RTC_DS1307 rtc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);  // refresh 60s

// ===== STRUCTS =====
struct MealWindow {
  bool valid = false;
  uint32_t startSec = 0;
  uint32_t endSec = 0;
};

MealWindow breakfast, lunchW, dinnerW;
String lastUpdatedAt = "";

// ===== TIMERS =====
unsigned long lastFetchMs = 0;
unsigned long lastRtcSyncMs = 0;

// ===== ALERT FLAGS =====
bool sent_m1 = false, sent_m2 = false, sent_e1 = false, sent_e2 = false, sent_n1 = false, sent_n2 = false;

// ===== CURRENT ALERT =====
String currentAlert = "";
unsigned long alertSetTime = 0;
const unsigned long ALERT_DISPLAY_MS = 10000;  // 10 seconds

// ===== HELPERS =====
uint32_t timeStringToSeconds(const String &t) {
  int hh = 0, mm = 0, ss = 0;
  sscanf(t.c_str(), "%02d:%02d:%02d", &hh, &mm, &ss);
  return (uint32_t)hh * 3600U + (uint32_t)mm * 60U + (uint32_t)ss;
}

uint32_t currentSecondsOfDay(const DateTime &dt) {
  return (uint32_t)dt.hour() * 3600U + (uint32_t)dt.minute() * 60U + (uint32_t)dt.second();
}

bool isNowInWindow(uint32_t nowSec, uint32_t startSec, uint32_t durationSec) {
  uint32_t endSec = (startSec + durationSec) % 86400U;
  if (startSec <= endSec) return nowSec >= startSec && nowSec <= endSec;
  return nowSec >= startSec || nowSec <= endSec;
}

void resetDailyFlags(uint16_t todayDay, uint16_t &savedDay) {
  if (todayDay != savedDay) {
    savedDay = todayDay;
    sent_m1 = sent_m2 = sent_e1 = sent_e2 = sent_n1 = sent_n2 = false;
  }
}

void sendAlert(const char *code) {
  Serial.print("[ALERT] ");
  Serial.print(code);
  Serial.print(" @ ");
  DateTime now = rtc.now();
  Serial.println(now.timestamp());
}

void wifiActivityBlink() {
  digitalWrite(LED_PIN, LOW);
  delay(50);
  digitalWrite(LED_PIN, HIGH);
}

// ===== FETCH TIMETABLE =====
void fetchTimetable() {
  if (WiFi.status() != WL_CONNECTED) return;

  IPAddress gw = WiFi.gatewayIP();
  String url = String("http://") + gw.toString() + "/medische/get_timetable.php";
  Serial.println("Fetching timetable: " + url);

  wifiActivityBlink();  // blink on request start

  HTTPClient http;
  WiFiClient client;
  http.begin(client, url);
  int code = http.GET();

  wifiActivityBlink();  // blink on response

  if (code == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
      Serial.print("JSON parse error: ");
      Serial.println(err.c_str());
    } else {
      if (String(doc["status"].as<const char *>()) == "success") {
        breakfast.valid = doc["timetable"]["breakfast"]["start"] ? true : false;
        lunchW.valid = doc["timetable"]["lunch"]["start"] ? true : false;
        dinnerW.valid = doc["timetable"]["dinner"]["start"] ? true : false;

        if (breakfast.valid) {
          breakfast.startSec = timeStringToSeconds(doc["timetable"]["breakfast"]["start"].as<String>());
          breakfast.endSec = timeStringToSeconds(doc["timetable"]["breakfast"]["end"].as<String>());
        }
        if (lunchW.valid) {
          lunchW.startSec = timeStringToSeconds(doc["timetable"]["lunch"]["start"].as<String>());
          lunchW.endSec = timeStringToSeconds(doc["timetable"]["lunch"]["end"].as<String>());
        }
        if (dinnerW.valid) {
          dinnerW.startSec = timeStringToSeconds(doc["timetable"]["dinner"]["start"].as<String>());
          dinnerW.endSec = timeStringToSeconds(doc["timetable"]["dinner"]["end"].as<String>());
        }

        lastUpdatedAt = doc["timetable"]["updated_at"].as<String>();
        Serial.println("Timetable updated: " + lastUpdatedAt);
      } else Serial.println("Server returned non-success status");
    }
  } else {
    Serial.printf("HTTP GET failed: %s\n", http.errorToString(code).c_str());
  }
  http.end();
}

// ===== CHECK MEAL ALERTS =====
void checkMeal(const MealWindow &mw, bool &sentBefore, bool &sentAfter, const char *beforeCode, const char *afterCode, uint32_t nowSec) {
  if (!mw.valid) return;
  uint32_t beforeSec = (mw.startSec + 86400U - BEFORE_MIN * 60U) % 86400U;
  uint32_t afterSec = mw.endSec;

  // Before meal: 15 min window
  if (!sentBefore && isNowInWindow(nowSec, beforeSec, BEFORE_MIN * 60UL)) {
    sendAlert(beforeCode);
    sentBefore = true;
    currentAlert = String("Take med: ") + beforeCode;
    alertSetTime = millis();
  }

  // After meal: 15 min window
  if (!sentAfter && isNowInWindow(nowSec, afterSec, AFTER_MIN * 60UL)) {
    sendAlert(afterCode);
    sentAfter = true;
    currentAlert = String("Take med: ") + afterCode;
    alertSetTime = millis();
  }
}


// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Booting...");
  lcd.setCursor(0, 1);
  lcd.print("Please wait");
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.printf("Connecting to %s ", ssid);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  Serial.println();

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.print("WiFi connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());
    Serial.println("Connected, IP: " + WiFi.localIP().toString());
  } else {
    lcd.setCursor(0, 0);
    lcd.print("WiFi failed");
    lcd.setCursor(0, 1);
    lcd.print("Check SSID/Pass");
    Serial.println("WiFi connect failed.");
  }
  delay(5000);

  // NTP and RTC
  timeClient.begin();
  timeClient.update();
  if (rtc.begin() && !rtc.isrunning()) {
    rtc.adjust(DateTime(timeClient.getEpochTime()));
    Serial.println("RTC set from NTP.");
  }

  lastFetchMs = millis() - FETCH_INTERVAL_MS;
  lastRtcSyncMs = millis();
}

// ===== LOOP =====
void loop() {
  unsigned long nowMs = millis();

  // Fetch timetable
  if (nowMs - lastFetchMs >= FETCH_INTERVAL_MS) {
    lastFetchMs = nowMs;
    fetchTimetable();
  }

  // RTC resync every hour
  if (nowMs - lastRtcSyncMs >= 60UL * 60UL * 1000UL) {
    lastRtcSyncMs = nowMs;
    timeClient.update();
    wifiActivityBlink();
    rtc.adjust(DateTime(timeClient.getEpochTime()));
    Serial.println("RTC resynced.");
  }

  // Current time
  DateTime now = rtc.now();
  if (!rtc.isrunning() && WiFi.status() == WL_CONNECTED) {
    timeClient.update();
    wifiActivityBlink();
    rtc.adjust(DateTime(timeClient.getEpochTime()));
  }

  // 12-hour format + timezone
  int hour24 = now.hour() + TIMEZONE_OFFSET;
  if (hour24 >= 24) hour24 -= 24;
  int displayHour = hour24 % 12;
  if (displayHour == 0) displayHour = 12;
  String ampm = (hour24 >= 12) ? "PM" : "AM";

  char buf[17];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d %s", displayHour, now.minute(), now.second(), ampm.c_str());
  lcd.setCursor(0, 0);
  lcd.print(buf);

  // Handle alert display
  if (currentAlert.length() > 0) {
    lcd.setCursor(0, 1);
    lcd.print(currentAlert);
    for (int i = currentAlert.length(); i < 16; i++) lcd.print(' ');
    if (millis() - alertSetTime > ALERT_DISPLAY_MS) currentAlert = "";
  } else {
    String line2 = lastUpdatedAt.length() ? ("Upd:" + lastUpdatedAt.substring(11)) : "No timetable";
    lcd.setCursor(0, 1);
    lcd.print(line2);
    for (int i = line2.length(); i < 16; i++) lcd.print(' ');
  }

  // Reset daily flags
  static uint16_t savedDay = 0;
  resetDailyFlags(now.day(), savedDay);

  uint32_t nowSec = currentSecondsOfDay(now);
  checkMeal(breakfast, sent_m1, sent_m2, "m1", "m2", nowSec);
  checkMeal(lunchW, sent_e1, sent_e2, "e1", "e2", nowSec);
  checkMeal(dinnerW, sent_n1, sent_n2, "n1", "n2", nowSec);

  delay(1000);
}

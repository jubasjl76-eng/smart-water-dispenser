/**
 * Smart Pet Water Dispenser Firmware
 * ESP32-based with Wi-Fi, scheduling, and API control
 * 
 * Features:
 * - Water pump control
 * - Water level monitoring (ultrasonic)
 * - Water quality sensor (TDS)
 * - Temperature sensor
 * - Wi-Fi connectivity
 * - Scheduled water dispensing
 * - Safety fail-safes
 * - Event logging
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// ============== CONFIGURATION ==============
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* API_BASE_URL = "http://localhost:3003/api";
const char* API_KEY = "your-api-key-here";

// Hardware Pins
const int PUMP_PIN = 4;
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int TDS_PIN = 34;  // Analog pin
const int TEMP_PIN = 35; // Analog pin (thermistor)
const int LED_PIN = 2;

// ============== CONSTANTS ==============
const int PUMP_ON_TIME_MS = 5000; // Default pump runtime
const int MAX_WATER_LEVEL_CM = 15;
const int MIN_WATER_LEVEL_CM = 2;
const int LOW_WATER_THRESHOLD = 20; // percentage

// TDS thresholds (ppm)
const int TDS_GOOD = 300;
const int TDS_ACCEPTABLE = 600;

// ============== GLOBALS ==============
Preferences preferences;
bool isConnected = false;
unsigned long lastApiCall = 0;
const unsigned long API_CALL_INTERVAL = 30000;

struct FeedingSchedule {
  int hour;
  int minute;
  bool enabled;
};

#define MAX_SCHEDULES 5
FeedingSchedule schedules[MAX_SCHEDULES];
int scheduleCount = 0;

#define MAX_LOG_ENTRIES 50
String eventLog[MAX_LOG_ENTRIES];
int logIndex = 0;
int logCount = 0;

// ============== PUMP CONTROL ==============
/**
 * Initialize pump
 */
void initPump() {
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  logEvent("Pump initialized");
}

/**
 * Turn on water pump
 * @param durationMs Duration in milliseconds
 * @return true if successful
 */
bool pumpWater(int durationMs = PUMP_ON_TIME_MS) {
  if (!isPumpSafe()) {
    logEvent("ERROR: Pump not safe to operate");
    return false;
  }
  
  logEvent("Pump started...");
  digitalWrite(PUMP_PIN, HIGH);
  delay(durationMs);
  digitalWrite(PUMP_PIN, LOW);
  
  logEvent("Pump stopped");
  return true;
}

/**
 * Stop pump immediately
 */
void stopPump() {
  digitalWrite(PUMP_PIN, LOW);
  logEvent("Pump emergency stop");
}

// ============== WATER LEVEL SENSOR ==============
/**
 * Measure water level using ultrasonic sensor
 */
float measureWaterLevel() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * 0.0343) / 2;
  
  if (distance > 0 && distance < 400) {
    return distance;
  }
  return -1;
}

/**
 * Get water level percentage (0-100)
 */
int getWaterLevel() {
  float distance = measureWaterLevel();
  if (distance < 0) return -1;
  
  int level = map(distance, MIN_WATER_LEVEL_CM, MAX_WATER_LEVEL_CM, 100, 0);
  return constrain(level, 0, 100);
}

/**
 * Check if water level is low
 */
bool isWaterLow() {
  return getWaterLevel() < LOW_WATER_THRESHOLD;
}

/**
 * Check if pump operation is safe
 */
bool isPumpSafe() {
  float distance = measureWaterLevel();
  return distance > MIN_WATER_LEVEL_CM;
}

// ============== TDS SENSOR ==============
/**
 * Read TDS value (ppm)
 * Note: Requires calibration based on your sensor
 */
int readTDS() {
  int rawValue = analogRead(TDS_PIN);
  // Convert to ppm (approximate - needs calibration)
  // Typical: 0-1000 ppm range
  int tds = map(rawValue, 0, 4095, 0, 1000);
  return tds;
}

/**
 * Check water quality
 * @return 0 = good, 1 = acceptable, 2 = poor
 */
int getWaterQuality() {
  int tds = readTDS();
  if (tds < TDS_GOOD) return 0; // Good
  if (tds < TDS_ACCEPTABLE) return 1; // Acceptable
  return 2; // Poor
}

// ============== TEMPERATURE SENSOR ==============
/**
 * Read temperature in Celsius
 * Note: Requires thermistor calibration
 */
float readTemperature() {
  int rawValue = analogRead(TEMP_PIN);
  // Convert to Celsius (example for 10k thermistor)
  // Needs calibration for your specific thermistor
  float voltage = rawValue * (3.3 / 4095.0);
  float resistance = 10000 / ((3.3 / voltage) - 1);
  float temperature = 1.0 / (1.0 / 298.15 + 1.0 / 3435.0 * log(resistance / 10000)) - 273.15;
  return temperature;
}

// ============== WIFI ==============
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    isConnected = true;
    logEvent("WiFi connected");
  } else {
    isConnected = false;
    logEvent("ERROR: WiFi connection failed");
  }
}

// ============== API ==============
void sendStatusToApi() {
  if (!isConnected) return;
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/status";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);
  
  StaticJsonDocument<512> doc;
  doc["device_id"] = preferences.getString("device_id", "water_001");
  doc["water_level"] = getWaterLevel();
  doc["is_low_water"] = isWaterLow();
  doc["tds"] = readTDS();
  doc["temperature"] = readTemperature();
  doc["water_quality"] = getWaterQuality();
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["uptime_ms"] = millis();
  
  String json;
  serializeJson(doc, json);
  
  int httpCode = http.POST(json);
  
  if (httpCode == HTTP_CODE_OK) {
    logEvent("Status sent to API");
  } else {
    logEvent("ERROR: Failed to send status");
  }
  
  http.end();
}

void fetchScheduleFromApi() {
  if (!isConnected) return;
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/schedule";
  
  http.begin(url);
  http.addHeader("X-API-Key", API_KEY);
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      scheduleCount = 0;
      JsonArray scheduleArray = doc["schedules"];
      for (JsonObject sched : scheduleArray) {
        if (scheduleCount < MAX_SCHEDULES) {
          schedules[scheduleCount].hour = sched["hour"];
          schedules[scheduleCount].minute = sched["minute"];
          schedules[scheduleCount].enabled = sched["enabled"];
          scheduleCount++;
        }
      }
      logEvent("Schedule updated from API");
    }
  }
  
  http.end();
}

void handleApiCommand(const String& command) {
  if (command == "pump") {
    pumpWater();
  } else if (command == "pump_on") {
    digitalWrite(PUMP_PIN, HIGH);
    logEvent("Pump manually started");
  } else if (command == "pump_off") {
    stopPump();
  } else if (command == "status") {
    Serial.printf("Water Level: %d%%\n", getWaterLevel());
    Serial.printf("TDS: %d ppm\n", readTDS());
    Serial.printf("Temp: %.1f C\n", readTemperature());
  }
}

// ============== SCHEDULING ==============
void checkSchedule() {
  if (scheduleCount == 0) return;
  
  static int lastMinute = -1;
  int currentMinute = minute();
  
  if (currentMinute != lastMinute) {
    lastMinute = currentMinute;
    
    for (int i = 0; i < scheduleCount; i++) {
      if (schedules[i].enabled && 
          schedules[i].hour == hour() && 
          schedules[i].minute == minute()) {
        logEvent("Scheduled water dispense triggered");
        pumpWater();
      }
    }
  }
}

// ============== LOGGING ==============
void logEvent(const String& message) {
  String entry = "[" + String(millis()) + "] " + message;
  eventLog[logIndex] = entry;
  logIndex = (logIndex + 1) % MAX_LOG_ENTRIES;
  if (logCount < MAX_LOG_ENTRIES) logCount++;
  Serial.println(entry);
}

String getLogJson() {
  String json = "{\"logs\":[";
  int start = (logCount < MAX_LOG_ENTRIES) ? 0 : logIndex;
  
  for (int i = 0; i < logCount; i++) {
    int idx = (start + i) % MAX_LOG_ENTRIES;
    json += "\"" + eventLog[idx] + "\"";
    if (i < logCount - 1) json += ",";
  }
  
  json += "]}";
  return json;
}

// ============== LED ==============
void updateLED() {
  if (!isConnected) {
    digitalWrite(LED_PIN, (millis() / 1000) % 2);
  } else if (isWaterLow()) {
    digitalWrite(LED_PIN, (millis() / 200) % 2);
  } else if (getWaterQuality() == 2) {
    digitalWrite(LED_PIN, (millis() / 300) % 2);
  } else {
    digitalWrite(LED_PIN, HIGH);
  }
}

// ============== SETUP ==============
void setup() {
  Serial.begin(115200);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  
  preferences.begin("smart-water", false);
  
  initPump();
  connectWiFi();
  
  logEvent("Smart Water Dispenser initialized");
  
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}

// ============== LOOP ==============
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    isConnected = false;
    connectWiFi();
  }
  
  if (millis() - lastApiCall > API_CALL_INTERVAL) {
    lastApiCall = millis();
    sendStatusToApi();
    fetchScheduleFromApi();
  }
  
  checkSchedule();
  updateLED();
  
  delay(100);
}

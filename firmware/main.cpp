/**
 * Smart Pet Water Dispenser — firmware on smart-pet-device-sdk.
 *
 * The SDK owns Wi-Fi + SoftAP provisioning, NTP, MQTT on
 * kennel/{kennelId}/water/{deviceId}/*, LWT, OTA, command/ack, schedule caching
 * and the offline journal. This file is just the dispenser: run the pump on
 * `dispense` / schedule, report level / TDS / temperature, a status LED.
 *
 * First boot with no NVS creds: the device opens the "smartpet-<mac>" Wi-Fi AP;
 * connect and fill in the form. Nothing is hardcoded.
 *
 * The pre-SDK single-file firmware is kept as smart-water-dispenser.legacy.cpp.
 *
 * Wiring (diagram.json): pump relay GPIO4, ultrasonic TRIG 5 / ECHO 18,
 * TDS analog GPIO34, NTC thermistor analog GPIO35, status LED GPIO2.
 */
#include <SmartPetDevice.h>

constexpr int PUMP_PIN = 4;
constexpr int TRIG_PIN = 5;
constexpr int ECHO_PIN = 18;
constexpr int TDS_PIN  = 34;
constexpr int NTC_PIN  = 35;
constexpr int LED_PIN  = 2;

constexpr float DEFAULT_DISPENSE_S = 5.0f;
constexpr float LOW_WATER_PCT      = 20.0f;
constexpr float MIN_SAFE_PCT       = 5.0f;    // below this the tank is dry — do not run the pump
constexpr float TDS_GOOD_PPM       = 300.0f;
constexpr float TDS_OK_PPM         = 600.0f;

spd::SmartPetDevice dev("water");
spd::WaterModule    water(PUMP_PIN, TRIG_PIN, ECHO_PIN, TDS_PIN, NTC_PIN);

// 0 good, 1 acceptable, 2 poor
static int quality(float tds) { return tds < TDS_GOOD_PPM ? 0 : tds < TDS_OK_PPM ? 1 : 2; }

static void dispenseNow(float seconds) {
  if (water.levelPct() < MIN_SAFE_PCT) {
    Serial.println("[water] tank low — skipping pump");
    dev.reportAction("dispense_skipped", 0);
    return;
  }
  digitalWrite(LED_PIN, HIGH);
  water.dispense(seconds);
  digitalWrite(LED_PIN, LOW);
  dev.reportAction("dispensed", seconds);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  water.begin();
  dev.begin();

  // Timed dispense from the backend or a schedule.
  dev.onCommand("dispense", [](JsonObjectConst p, const String&) {
    dispenseNow(p["seconds"] | p["amount"] | DEFAULT_DISPENSE_S);
    return true;
  });
  // Manual hold-open / stop.
  dev.onCommand("pump_on",  [](JsonObjectConst, const String&) { water.start(); return true; });
  dev.onCommand("pump_off", [](JsonObjectConst, const String&) { water.stop();  return true; });

  dev.onScheduledAction([](float seconds) {
    dispenseNow(seconds > 0 ? seconds : DEFAULT_DISPENSE_S);
  });

  dev.onStatusFill([](JsonObject& s) {
    float lvl = water.levelPct();
    float tds = water.tdsPpm(3.3f, water.temperatureC());
    s["waterLevel"]   = lvl;
    s["isLowWater"]   = lvl < LOW_WATER_PCT;
    s["tds"]          = tds;
    s["temperatureC"] = water.temperatureC();
    s["quality"]      = quality(tds);
  });

  dev.identifyFn_ = [](int secs) {
    for (int i = 0; i < secs * 2; ++i) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(250);
    }
    digitalWrite(LED_PIN, LOW);
  };
}

// Status LED: fast blink = provisioning/offline, slow = low water,
// medium = poor quality, on = ok.
static void updateLed() {
  if (dev.inProvisioning() || !dev.isOnline()) {
    digitalWrite(LED_PIN, (millis() / 300) % 2);
  } else if (water.levelPct() < LOW_WATER_PCT) {
    digitalWrite(LED_PIN, (millis() / 1000) % 2);
  } else if (quality(water.tdsPpm()) == 2) {
    digitalWrite(LED_PIN, (millis() / 500) % 2);
  } else {
    digitalWrite(LED_PIN, HIGH);
  }
}

void loop() {
  dev.loop();
  updateLed();

  static uint32_t lastPub = 0;
  if (dev.isOnline() && millis() - lastPub > 60000) {
    lastPub = millis();
    dev.publishMetric("level", water.levelPct(), "percent");
    dev.publishMetric("tds", water.tdsPpm(), "ppm");
    dev.publishMetric("temp", water.temperatureC(), "celsius");
  }

  delay(10);
}

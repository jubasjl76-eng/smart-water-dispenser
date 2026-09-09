# Water dispenser firmware

`main.cpp` on [smart-pet-device-sdk](https://github.com/jubasjl76-eng/smart-pet-device-sdk).
The SDK handles Wi-Fi/SoftAP provisioning, NTP, MQTT, LWT, OTA, command/ack,
schedule caching and the offline journal. This sketch adds: run the pump on
`dispense` / schedule, `pump_on` / `pump_off` for a manual hold, level / TDS /
temperature on status, a status LED, and a dry-tank guard.

## Build

```bash
pio run -d firmware
```

`platformio.ini` pins the SDK to a commit, forces `-std=gnu++17`, and uses
`min_spiffs.csv` so signed A/B OTA has two ~1.9 MB app slots.

## Wiring (diagram.json)

| GPIO | part |
|---|---|
| 4  | pump relay |
| 5  | HC-SR04 TRIG |
| 18 | HC-SR04 ECHO |
| 34 | TDS probe (analog) |
| 35 | NTC thermistor divider (analog) |
| 2  | status LED |

## First boot / provisioning

No credentials are compiled in. On first boot the device opens a Wi-Fi AP
`smartpet-<mac>`; join it and fill in kennelId, deviceId, Wi-Fi, MQTT host and
the claim password. Values persist in NVS.

## Calibrate on a real board

The simulator can't give these:

| what | how |
|---|---|
| level `fullCm` / `emptyCm` in `WaterModule::levelPct` | measure the HC-SR04 distance with the tank full and empty. Defaults 2 cm / 15 cm. |
| TDS ppm | put the probe in a reference solution of known ppm and adjust; the SDK curve is generic. |
| NTC `beta` / `r0` | from the thermistor datasheet, or a two-point measurement. Defaults 3950 / 10 kΩ. |
| pump seconds → mL | time the pump into a measuring jug to know how much a `dispense` delivers. |

Also verify on hardware: NTP + schedule across a reboot, captive portal on a
phone, an OTA pull end to end.

## CI

- `pio run -d firmware` builds for `esp32dev` on every push (via
  `smart-pet-ci/pio-ci`).
- `firmware/water.test.yaml` is a Wokwi smoke test (boot → provisioning portal
  opens). It runs when a `WOKWI_CLI_TOKEN` repo secret is set; free token at
  <https://wokwi.com/dashboard/ci>.

# Smart Pet Water Dispenser

A Wi-Fi enabled smart pet water dispenser with ESP32, water quality monitoring, scheduled dispensing, and REST API control.

## Features

- 🌐 **Wi-Fi Connected** - Control from anywhere via API
- 💧 **Water Level Monitoring** - Ultrasonic sensor for level tracking
- 🔬 **Water Quality (TDS)** - Total Dissolved Solids monitoring
- 🌡️ **Temperature Monitoring** - Real-time water temperature
- ⏰ **Scheduled Dispensing** - Set multiple feeding times
- 🔒 **API Authentication** - Secure API key based access
- 📝 **Event Logging** - Track all water events

## Hardware

### Components

| Component | Model | Cost (€) |
|-----------|-------|----------|
| Microcontroller | ESP32 DevKit V1 | 10 |
| Water Pump | 5V DC submersible | 5 |
| Ultrasonic Sensor | HC-SR04 | 3 |
| TDS Sensor | TDS Meter Kit | 5 |
| Thermistor | 10K NTC | 2 |
| Power Supply | 5V 3A | 8 |
| Misc | - | 5 |

**Total: ~€38**

### Pin Configuration

| Pin | Component |
|-----|-----------|
| 4 | Water Pump |
| 5 | Ultrasonic Trig |
| 18 | Ultrasonic Echo |
| 34 | TDS Sensor (Analog) |
| 35 | Temperature (Analog) |
| 2 | LED Indicator |

## Software

### Firmware

Location: `firmware/main.cpp` — built on
[smart-pet-device-sdk](https://github.com/jubasjl76-eng/smart-pet-device-sdk).
The SDK owns Wi-Fi + SoftAP provisioning, NTP, MQTT
(`kennel/{kennelId}/water/{deviceId}/*`), LWT, OTA, command/ack, schedule
caching and the offline journal; `main.cpp` is just the dispenser. See
`firmware/README.md` for build / flash / calibration.

```bash
pio run -d firmware
```

The pre-SDK single-file firmware is kept as
`firmware/smart-water-dispenser.legacy.cpp`.

### Backend

There is no per-device backend any more. The dispenser talks MQTT to
**[smart-pet-backend](https://github.com/jubasjl76-eng/smart-pet-backend)**.
The legacy `backend/` folder in this repo is dead and will be removed.

## 3D Design

Location: `3d-design/enclosure.scad`

Open in OpenSCAD to view and export STL files.

## Getting Started

1. Order components (~€38)
2. 3D print enclosure
3. Assemble hardware
4. `pio run -d firmware` and flash
5. Join the `smartpet-<mac>` AP on first boot and provision
6. Claim the device from the Smart Pet console; set a schedule

## License

MIT

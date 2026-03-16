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

Location: `firmware/smart-water-dispenser.cpp`

### Backend API

Location: `backend/`

```bash
cd backend
npm install
npm run dev
```

API runs on http://localhost:3003

## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | /health | Health check |
| GET | /api/devices | List all devices |
| POST | /api/devices | Register new device |
| POST | /api/status | Update device status |
| POST | /api/dispense | Trigger water dispense |
| GET | /api/schedule | Get schedules |
| POST | /api/schedule | Create schedule |

**Required Header:** `X-API-Key: your-api-key-here`

## 3D Design

Location: `3d-design/enclosure.scad`

Open in OpenSCAD to view and export STL files.

## Getting Started

1. Order components (~€38)
2. 3D print enclosure
3. Assemble hardware
4. Flash firmware
5. Start backend API
6. Register device
7. Set schedule

## Wokwi Simulation

This firmware can be simulated in Wokwi without physical hardware.

### Simulated Hardware Components

- **ESP32 DevKit V1** - Main microcontroller
- **Ultrasonic Sensor** - Water level monitoring
- **Relay** - Pump control
- **LED** - Status indicator (blue)
- **Potentiometer (TDS)** - Water quality simulation
- **Potentiometer (Temp)** - Temperature simulation
- **Push Button** - Manual dispense trigger

### Running the Simulation

1. Open [Wokwi](https://wokwi.com)
2. Create a new ESP32 project
3. Upload the `firmware/diagram.json` file
4. Build the firmware:
   ```bash
   # Using PlatformIO (if available)
   pio run
   ```
5. Upload the compiled firmware to Wokwi
6. The simulation will start automatically

### Pin Connections

| ESP32 Pin | Component |
|-----------|-----------|
| 4 | Pump Relay |
| 5 | Ultrasonic Trig |
| 18 | Ultrasonic Echo |
| 34 | TDS Sensor (analog) |
| 35 | Temperature (analog) |
| 2 | Status LED |
| 0 | Push Button |

### Testing

The simulation will show:
- Pump relay activation
- Ultrasonic sensor measuring water level
- TDS value simulation
- Temperature value simulation
- LED status indication

## License

MIT

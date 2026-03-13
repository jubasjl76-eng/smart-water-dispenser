/**
 * Smart Water Dispenser Backend API
 * Node.js + Express + TypeScript
 * 
 * Features:
 * - API key authentication
 * - Multiple device management
 * - Water quality monitoring (TDS)
 * - Temperature monitoring
 * - Water level tracking
 * - Scheduling
 * - Event logging
 */

import express, { Express, Request, Response, NextFunction } from 'express';
import cors from 'cors';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import crypto from 'crypto';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app: Express = express();
const PORT = 3003;
const DATA_DIR = path.join(__dirname, 'data');

// ============== TYPES ==============
interface WaterDevice {
  id: string;
  name: string;
  waterLevel: number;
  isLowWater: boolean;
  tds: number;
  temperature: number;
  waterQuality: number; // 0=good, 1=acceptable, 2=poor
  wifiRssi: number;
  uptimeMs: number;
  lastSeen: string;
  createdAt: string;
}

interface Schedule {
  id: string;
  deviceId: string;
  hour: number;
  minute: number;
  enabled: boolean;
  createdAt: string;
}

interface WaterEvent {
  id: string;
  deviceId: string;
  type: 'scheduled' | 'manual' | 'api' | 'low_water' | 'poor_quality';
  timestamp: string;
  success: boolean;
  message?: string;
}

interface ApiLog {
  id: string;
  deviceId: string;
  endpoint: string;
  method: string;
  statusCode: number;
  timestamp: string;
}

// ============== DATA STORAGE ==============
interface Database {
  devices: WaterDevice[];
  schedules: Schedule[];
  events: WaterEvent[];
  logs: ApiLog[];
}

let db: Database = {
  devices: [],
  schedules: [],
  events: [],
  logs: []
};

const DB_FILE = path.join(DATA_DIR, 'database.json');

if (!fs.existsSync(DATA_DIR)) {
  fs.mkdirSync(DATA_DIR, { recursive: true });
}

if (fs.existsSync(DB_FILE)) {
  try {
    const data = fs.readFileSync(DB_FILE, 'utf-8');
    db = JSON.parse(data);
  } catch (err) {
    console.error('Failed to load database:', err);
  }
}

const saveDb = (): void => {
  fs.writeFileSync(DB_FILE, JSON.stringify(db, null, 2));
};

// ============== MIDDLEWARE ==============
app.use(cors());
app.use(express.json());

const API_KEY = process.env.API_KEY || 'your-api-key-here';

const authMiddleware = (req: Request, res: Response, next: NextFunction): void => {
  const providedKey = req.headers['x-api-key'] as string;
  
  if (!providedKey || providedKey !== API_KEY) {
    res.status(401).json({ error: 'Unauthorized: Invalid API key' });
    return;
  }
  
  next();
};

// ============== HEALTH CHECK ==============
app.get('/health', (_req: Request, res: Response) => {
  res.json({ 
    status: 'ok', 
    timestamp: new Date().toISOString(),
    uptime: process.uptime()
  });
});

// ============== DEVICE ENDPOINTS ==============

// Get all devices
app.get('/api/devices', authMiddleware, (_req: Request, res: Response) => {
  res.json({ devices: db.devices });
});

// Get single device
app.get('/api/devices/:id', authMiddleware, (req: Request, res: Response) => {
  const device = db.devices.find(d => d.id === req.params.id);
  
  if (!device) {
    res.status(404).json({ error: 'Device not found' });
    return;
  }
  
  res.json(device);
});

// Register new device
app.post('/api/devices', authMiddleware, (req: Request, res: Response) => {
  const { name } = req.body;
  
  const device: WaterDevice = {
    id: `water_${crypto.randomBytes(4).toString('hex')}`,
    name: name || 'Smart Water Dispenser',
    waterLevel: 100,
    isLowWater: false,
    tds: 0,
    temperature: 0,
    waterQuality: 0,
    wifiRssi: 0,
    uptimeMs: 0,
    lastSeen: new Date().toISOString(),
    createdAt: new Date().toISOString()
  };
  
  db.devices.push(device);
  saveDb();
  
  res.status(201).json(device);
});

// Update device status
app.post('/api/status', authMiddleware, (req: Request, res: Response) => {
  const { 
    device_id, 
    water_level, 
    is_low_water, 
    tds, 
    temperature,
    water_quality,
    wifi_rssi, 
    uptime_ms 
  } = req.body;
  
  let device = db.devices.find(d => d.id === device_id);
  
  if (!device) {
    device = {
      id: device_id,
      name: 'Smart Water Dispenser',
      waterLevel: water_level,
      isLowWater: is_low_water,
      tds: tds || 0,
      temperature: temperature || 0,
      waterQuality: water_quality || 0,
      wifiRssi: wifi_rssi,
      uptimeMs: uptime_ms,
      lastSeen: new Date().toISOString(),
      createdAt: new Date().toISOString()
    };
    db.devices.push(device);
  } else {
    device.waterLevel = water_level;
    device.isLowWater = is_low_water;
    device.tds = tds || device.tds;
    device.temperature = temperature || device.temperature;
    device.waterQuality = water_quality || device.waterQuality;
    device.wifiRssi = wifi_rssi;
    device.uptimeMs = uptime_ms;
    device.lastSeen = new Date().toISOString();
  }
  
  // Log the API call
  const logEntry: ApiLog = {
    id: `log_${Date.now()}`,
    deviceId: device_id,
    endpoint: '/api/status',
    method: 'POST',
    statusCode: 200,
    timestamp: new Date().toISOString()
  };
  db.logs.push(logEntry);
  
  if (db.logs.length > 100) {
    db.logs = db.logs.slice(-100);
  }
  
  saveDb();
  res.json({ success: true });
});

// ============== WATER CONTROL ENDPOINTS ==============

// Trigger water dispense
app.post('/api/dispense', authMiddleware, (req: Request, res: Response) => {
  const { device_id, type = 'api', duration_ms } = req.body;
  
  const device = db.devices.find(d => d.id === device_id);
  
  if (!device) {
    res.status(404).json({ error: 'Device not found' });
    return;
  }
  
  const event: WaterEvent = {
    id: `event_${Date.now()}`,
    deviceId: device_id,
    type: type as 'scheduled' | 'manual' | 'api',
    timestamp: new Date().toISOString(),
    success: true,
    message: duration_ms ? `Dispensing for ${duration_ms}ms` : 'Water dispense triggered'
  };
  
  db.events.push(event);
  
  if (db.events.length > 100) {
    db.events = db.events.slice(-100);
  }
  
  saveDb();
  res.json({ success: true, event });
});

// Get water quality history
app.get('/api/events/:deviceId', authMiddleware, (req: Request, res: Response) => {
  const events = db.events.filter(e => e.deviceId === req.params.deviceId);
  res.json({ events });
});

// ============== SCHEDULE ENDPOINTS ==============

// Get schedules
app.get('/api/schedule', authMiddleware, (req: Request, res: Response) => {
  const deviceId = req.query.device_id as string;
  
  let schedules = db.schedules;
  if (deviceId) {
    schedules = db.schedules.filter(s => s.deviceId === deviceId);
  }
  
  res.json({ schedules });
});

// Create schedule
app.post('/api/schedule', authMiddleware, (req: Request, res: Response) => {
  const { device_id, hour, minute, enabled = true } = req.body;
  
  if (!device_id || hour === undefined || minute === undefined) {
    res.status(400).json({ error: 'Missing required fields' });
    return;
  }
  
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    res.status(400).json({ error: 'Invalid time values' });
    return;
  }
  
  const schedule: Schedule = {
    id: `sched_${crypto.randomBytes(4).toString('hex')}`,
    deviceId: device_id,
    hour,
    minute,
    enabled,
    createdAt: new Date().toISOString()
  };
  
  db.schedules.push(schedule);
  saveDb();
  
  res.status(201).json(schedule);
});

// Update schedule
app.put('/api/schedule/:id', authMiddleware, (req: Request, res: Response) => {
  const { hour, minute, enabled } = req.body;
  
  const schedule = db.schedules.find(s => s.id === req.params.id);
  
  if (!schedule) {
    res.status(404).json({ error: 'Schedule not found' });
    return;
  }
  
  if (hour !== undefined) {
    if (hour < 0 || hour > 23) {
      res.status(400).json({ error: 'Invalid hour' });
      return;
    }
    schedule.hour = hour;
  }
  
  if (minute !== undefined) {
    if (minute < 0 || minute > 59) {
      res.status(400).json({ error: 'Invalid minute' });
      return;
    }
    schedule.minute = minute;
  }
  
  if (enabled !== undefined) schedule.enabled = enabled;
  
  saveDb();
  res.json(schedule);
});

// Delete schedule
app.delete('/api/schedule/:id', authMiddleware, (req: Request, res: Response) => {
  const index = db.schedules.findIndex(s => s.id === req.params.id);
  
  if (index === -1) {
    res.status(404).json({ error: 'Schedule not found' });
    return;
  }
  
  db.schedules.splice(index, 1);
  saveDb();
  
  res.json({ success: true });
});

// ============== LOGGING ENDPOINTS ==============

// Get API logs
app.get('/api/logs', authMiddleware, (_req: Request, res: Response) => {
  res.json({ logs: db.logs.slice(-50) });
});

// ============== ERROR HANDLING ==============
app.use((_req: Request, res: Response) => {
  res.status(404).json({ error: 'Not found' });
});

app.use((err: Error, _req: Request, res: Response, _next: NextFunction) => {
  console.error(err.stack);
  res.status(500).json({ error: 'Internal server error' });
});

// ============== START SERVER ==============
app.listen(PORT, () => {
  console.log(`Smart Water Dispenser API running on http://localhost:${PORT}`);
  console.log(`API Key: ${API_KEY}`);
  console.log('');
  console.log('Endpoints:');
  console.log('  GET  /health                  - Health check');
  console.log('  GET  /api/devices             - List all devices');
  console.log('  POST /api/devices             - Register new device');
  console.log('  POST /api/status              - Update device status');
  console.log('  POST /api/dispense           - Trigger water dispense');
  console.log('  GET  /api/schedule            - Get schedules');
  console.log('  POST /api/schedule           - Create schedule');
  console.log('  PUT  /api/schedule/:id       - Update schedule');
  console.log('  DELETE /api/schedule/:id     - Delete schedule');
  console.log('');
  console.log('Headers required: X-API-Key: your-api-key-here');
});

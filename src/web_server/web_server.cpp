#include "web_server.h"
#include <Arduino.h>

void WebService::begin() {
    Serial.println("Starting Web Server for 3-Beacon Positioning System...");

    // Настройка WebSocket
    webSocket.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onWebSocketEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&webSocket);

    setupRoutes();

    server.begin();
    Serial.println("Web Server started on port 80");
    Serial.println("Open: http://" + WiFi.softAPIP().toString());
}

void WebService::setupRoutes() {
    // Главная страница с радаром
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(200, "text/html", getHTMLPage());
    });

    // API для получения данных системы
    server.on("/system", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String json = getJSONSystemData();
        request->send(200, "application/json", json);
    });

    // Старт измерений
    server.on("/start", HTTP_GET, [this](AsyncWebServerRequest* request) {
        measurementsActive = true;
        activeBeaconId = 1; // Начинаем с маяка 1
        request->send(200, "application/json", "{\"status\":\"started\"}");
        Serial.println("Measurements STARTED via web");
        broadcastSystemData();
    });

    // Стоп измерений
    server.on("/stop", HTTP_GET, [this](AsyncWebServerRequest* request) {
        measurementsActive = false;
        activeBeaconId = -1;
        detectedSensors = 0;
        request->send(200, "application/json", "{\"status\":\"stopped\"}");
        Serial.println("Measurements STOPPED via web");
        broadcastSystemData();
    });

    // Калибровка системы
    server.on("/calibrate", HTTP_GET, [this](AsyncWebServerRequest* request) {
        currentPosition.valid = false;
        activeBeaconId = -1;
        detectedSensors = 0;
        request->send(200, "application/json", "{\"status\":\"calibrated\"}");
        Serial.println("System CALIBRATED via web");
        broadcastSystemData();
    });

    // Статус системы
    server.on("/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String status = "{\"measuring\":" + String(measurementsActive ? "true" : "false") +
                       ",\"active_beacon\":" + String(activeBeaconId) +
                       ",\"detected_sensors\":" + String(detectedSensors) + "}";
        request->send(200, "application/json", status);
    });

    // Обработка 404
    server.onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "text/plain", "Page not found");
    });
}

void WebService::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch(type) {
        case WS_EVT_CONNECT:
            Serial.println("WebSocket client connected");
            // Отправляем текущие данные при подключении
            client->text(getJSONSystemData());
            break;

        case WS_EVT_DISCONNECT:
            Serial.println("WebSocket client disconnected");
            break;

        case WS_EVT_DATA:
            if (len > 0) {
                String message = String((char*)data).substring(0, len);
                Serial.println("WebSocket command: " + message);

                if (message == "START") {
                    measurementsActive = true;
                    activeBeaconId = 1;
                    client->text("{\"type\":\"status\",\"message\":\"Measurements started\"}");
                    Serial.println("Measurements STARTED via WebSocket");
                    broadcastSystemData();
                } else if (message == "STOP") {
                    measurementsActive = false;
                    activeBeaconId = -1;
                    detectedSensors = 0;
                    client->text("{\"type\":\"status\",\"message\":\"Measurements stopped\"}");
                    Serial.println("Measurements STOPPED via WebSocket");
                    broadcastSystemData();
                } else if (message == "CALIBRATE") {
                    currentPosition.valid = false;
                    activeBeaconId = -1;
                    detectedSensors = 0;
                    client->text("{\"type\":\"status\",\"message\":\"System calibrated\"}");
                    Serial.println("System CALIBRATED via WebSocket");
                    broadcastSystemData();
                }
            }
            break;

        case WS_EVT_ERROR:
            Serial.println("WebSocket error");
            break;
    }
}

String WebService::getJSONSystemData() {
    StaticJsonDocument<512> doc;

    // Данные источника (объекта)
    JsonObject source = doc.createNestedObject("source");
    source["x"] = currentPosition.x;
    source["y"] = currentPosition.y;
    source["accuracy"] = currentPosition.accuracy;
    source["valid"] = currentPosition.valid;

    // Системные данные
    doc["active_beacon"] = activeBeaconId;
    doc["detected_sensors"] = detectedSensors;
    doc["measuring"] = measurementsActive;
    doc["timestamp"] = millis();

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

void WebService::broadcastSystemData() {
    String json = getJSONSystemData();
    webSocket.textAll(json);
}

// ПРОСТАЯ РЕАЛИЗАЦИЯ ДАТЧИКОВ
int WebService::readRealSensors() {
    // Пока возвращаем 0 для простоты
    // В реальной системе здесь будет чтение с пинов
    return 0;
}

void WebService::updatePosition(const PositionData& newPosition) {
    currentPosition = newPosition;

    if (currentPosition.valid) {
        // Определяем ближайший маяк из ТРЕХ
        float distToBeacon1 = sqrt(pow(currentPosition.x - BEACON1_X, 2) + pow(currentPosition.y - BEACON1_Y, 2));
        float distToBeacon2 = sqrt(pow(currentPosition.x - BEACON2_X, 2) + pow(currentPosition.y - BEACON2_Y, 2));
        float distToBeacon3 = sqrt(pow(currentPosition.x - BEACON3_X, 2) + pow(currentPosition.y - BEACON3_Y, 2));

        // Находим минимальное расстояние
        float minDist = min(distToBeacon1, min(distToBeacon2, distToBeacon3));

        if (minDist == distToBeacon1) activeBeaconId = 1;
        else if (minDist == distToBeacon2) activeBeaconId = 2;
        else activeBeaconId = 3;

        // Простое чтение сенсоров (пока возвращает 0)
        detectedSensors = readRealSensors();

        Serial.println("3-Beacon System - Valid position detected");
        Serial.println("  Coordinates: X=" + String(currentPosition.x) + " Y=" + String(currentPosition.y));
        Serial.println("  Nearest beacon: " + String(activeBeaconId));
    } else {
        Serial.println("3-Beacon System - Position invalid or low accuracy");
        activeBeaconId = -1;
        detectedSensors = 0;
    }

    broadcastSystemData();
}

String WebService::getHTMLPage() {
    // Возвращаем ваш HTML код
    return R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Artak Positioning System</title>
  <style>
    * {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

html, body {
  width: 100%;
  height: 100%;
  overflow: hidden;
}

body {
  font-family: 'Courier New', 'Monaco', 'Consolas', monospace;
  background: #0a0f0a;
  color: #64ffa5;
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
}

body::before {
  content: '';
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background:
    linear-gradient(rgba(10, 15, 10, 0.9) 50%, rgba(0, 0, 0, 0.9) 50%);
  background-size: 100% 4px;
  z-index: -1;
  opacity: 0.1;
  pointer-events: none;
}

/* Эффект мерцания */
body::after {
  content: '';
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: transparent;
  animation: flicker 0.15s infinite;
  pointer-events: none;
  z-index: -1;
  opacity: 0.02;
}

@keyframes flicker {
  0%, 100% { opacity: 0.02; }
  50% { opacity: 0.05; }
}

.app-container {
  width: 100vw;
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: radial-gradient(ellipse at center, #0a1f0a 0%, #050a05 100%);
  position: relative;
}

.app-header {
  flex: 0 0 auto;
  padding: 15px 20px;
  text-align: center;
  background: rgba(5, 10, 5, 0.8);
  border-bottom: 2px solid #64ffa5;
  box-shadow: 0 0 20px rgba(100, 255, 165, 0.3);
}

h1 {
  font-size: 2.2rem;
  margin-bottom: 15px;
  color: #64ffa5;
  text-shadow:
    0 0 10px #64ffa5,
    0 0 20px #64ffa5;
  letter-spacing: 3px;
  text-transform: uppercase;
  font-weight: 700;
}

.status-panel {
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 30px;
  flex-wrap: wrap;
}

.status {
  padding: 8px 20px;
  border-radius: 4px;
  font-size: 1rem;
  text-transform: uppercase;
  letter-spacing: 2px;
  border: 1px solid;
  font-family: 'Courier New', monospace;
  font-weight: 700;
  min-width: 180px;
}

.status.connected {
  background: rgba(100, 255, 165, 0.1);
  color: #64ffa5;
  border-color: #64ffa5;
  box-shadow:
    0 0 15px rgba(100, 255, 165, 0.5),
    inset 0 0 10px rgba(100, 255, 165, 0.2);
  text-shadow: 0 0 10px #64ffa5;
}

.status.disconnected {
  background: rgba(255, 50, 50, 0.1);
  color: #ff3232;
  border-color: #ff3232;
  box-shadow:
    0 0 15px rgba(255, 50, 50, 0.3),
    inset 0 0 10px rgba(255, 50, 50, 0.1);
  text-shadow: 0 0 10px #ff3232;
}

#coordinates, #systemInfo {
  font-size: 1rem;
  font-weight: 600;
  letter-spacing: 1px;
  text-transform: uppercase;
  padding: 8px 15px;
  background: rgba(10, 20, 10, 0.6);
  border-radius: 4px;
  border: 1px solid #64ffa5;
}

#coordinates {
  color: #64ffa5;
  text-shadow: 0 0 5px #64ffa5;
}

#systemInfo {
  color: #b7fe65;
  text-shadow: 0 0 5px #b7fe65;
}

.app-main {
  flex: 1 1 auto;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 20px;
  min-height: 0;
}

.radar-container {
  width: 100%;
  height: 100%;
  max-width: 100%;
  max-height: 100%;
  background: rgba(5, 10, 5, 0.9);
  padding: 15px;
  border-radius: 8px;
  border: 2px solid #64ffa5;
  box-shadow:
    0 0 40px rgba(100, 255, 165, 0.4),
    inset 0 0 40px rgba(100, 255, 165, 0.1);
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
}

.radar-container::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg,
  transparent,
  rgba(100, 255, 165, 0.1),
  transparent
  );
  animation: scan 3s linear infinite;
}

@keyframes scan {
  0% { left: -100%; }
  100% { left: 100%; }
}

#radarCanvas {
  width: 100%;
  height: 100%;
  background: #050a05;
  border-radius: 6px;
  border: 1px solid #64ffa5;
  box-shadow:
    inset 0 0 30px rgba(0, 0, 0, 0.8),
    0 0 30px rgba(100, 255, 165, 0.3);
  display: block;
}

.app-footer {
  flex: 0 0 auto;
  padding: 15px 20px;
  background: rgba(5, 10, 5, 0.8);
  border-top: 2px solid #64ffa5;
  box-shadow: 0 0 20px rgba(100, 255, 165, 0.3);
}

.controls {
  text-align: center;
}

button {
  background: rgba(10, 20, 10, 0.8);
  color: #64ffa5;
  border: 2px solid #64ffa5;
  padding: 12px 25px;
  margin: 0 15px;
  border-radius: 6px;
  font-size: 0.9rem;
  font-weight: 700;
  cursor: pointer;
  transition: all 0.3s ease;
  text-transform: uppercase;
  letter-spacing: 2px;
  font-family: 'Courier New', monospace;
  position: relative;
  overflow: hidden;
  box-shadow:
    0 0 15px rgba(100, 255, 165, 0.3),
    inset 0 0 10px rgba(100, 255, 165, 0.1);
}

button::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg,
  transparent,
  rgba(100, 255, 165, 0.2),
  transparent
  );
  transition: left 0.6s;
}

button:hover::before {
  left: 100%;
}

button:hover {
  background: rgba(100, 255, 165, 0.1);
  box-shadow:
    0 0 25px rgba(100, 255, 165, 0.6),
    inset 0 0 15px rgba(100, 255, 165, 0.2);
  text-shadow: 0 0 10px #64ffa5;
  transform: translateY(-2px);
}

button:active {
  transform: translateY(0);
}

@keyframes blink {
  0%, 100% { opacity: 1; }
  50% { opacity: 0; }
}

.status.connected::after {
  content: '|';
  animation: blink 1s infinite;
  margin-left: 5px;
}

@media (max-width: 768px) {
  .app-header {
    padding: 10px 15px;
  }

  h1 {
    font-size: 1.5rem;
    margin-bottom: 10px;
  }

  .status-panel {
    flex-direction: column;
    gap: 10px;
  }

  .status, #coordinates, #systemInfo {
    font-size: 0.9rem;
    min-width: auto;
    width: 100%;
    text-align: center;
  }

  .app-main {
    padding: 10px;
  }

  .radar-container {
    padding: 10px;
  }

  .app-footer {
    padding: 10px 15px;
  }

  button {
    padding: 10px 20px;
    margin: 5px;
    font-size: 0.8rem;
    width: calc(50% - 10px);
  }

  .controls {
    display: flex;
    flex-wrap: wrap;
    justify-content: center;
  }
}

@media (min-height: 1200px) {
  .app-header {
    padding: 20px 30px;
  }

  h1 {
    font-size: 2.5rem;
  }

  .status, #coordinates, #systemInfo {
    font-size: 1.1rem;
    padding: 12px 20px;
  }

  .app-footer {
    padding: 20px 30px;
  }

  button {
    padding: 15px 30px;
    font-size: 1rem;
  }
}
  </style>
</head>
<body>
<div class="app-container">
  <header class="app-header">
    <h1>ARTAK POSITIONING SYSTEM RADAR</h1>
    <div class="status-panel">
      <div id="status" class="status disconnected">SYSTEM OFFLINE</div>
      <div id="coordinates">SOURCE: X: 0.00, Y: 0.00</div>
      <div id="systemInfo">ACTIVE BEACON: -- | DETECTED SENSORS: 0</div>
    </div>
  </header>

  <main class="app-main">
    <div class="radar-container">
      <canvas id="radarCanvas"></canvas>
    </div>
  </main>

  <footer class="app-footer">
    <div class="controls">
      <button id="startBtn">START MEASUREMENT</button>
      <button id="stopBtn">STOP MEASUREMENT</button>
      <button id="calibrateBtn">CALIBRATE SYSTEM</button>
      <button id="resetViewBtn">RESET VIEW</button>
    </div>
  </footer>
</div>
<script>
// ========== КОНФИГУРАЦИЯ СИСТЕМЫ ==========
const SYSTEM_CONFIG = {
  PHYSICAL: {
    ROOM_WIDTH: 400,
    ROOM_HEIGHT: 300,
    SOUND_SPEED: 34300,
    MAX_DISTANCE: 500,
    ACCURACY_THRESHOLD: 50
  },

  NODE_POSITIONS: {
    RECEIVER: { x: 0, y: 0 },
    BEACON1: { x: -200, y: 300 },
    BEACON2: { x: 200, y: 300 },
    BEACON3: { x: 0, y: 0 }
  },

  NETWORK: {
    AP_SSID: "PositioningSystem",
    AP_PASSWORD: "password123",
    UDP_PORT: 1234,
    WEB_PORT: 80
  }
};

// ========== КОНФИГУРАЦИЯ ВИЗУАЛИЗАЦИИ ==========
const CONFIG = {
  CANVAS: {
    BACKGROUND: '#050a05',
    GRID_SIZE: 50,
    SCALE_FACTOR: 2.5,
    OFFSET_X: 500,
    OFFSET_Y: 350
  },
  COLORS: {
    SOURCE: '#ff4444',
    SOURCE_ACCURACY: 'rgba(255, 68, 68, 0.2)',
    BEACON_ACTIVE: '#64ffa5',
    BEACON_INACTIVE: '#333333',
    SENSOR_DETECTED: '#b7fe65',
    SENSOR_NORMAL: '#333333',
    RECEIVER: '#64ffa5',
    GRID: '#1a3a1a',
    PULSE: '#64ffa5'
  },
  SIZES: {
    SOURCE: 8,
    BEACON: 15,
    SENSOR: 8,
    RECEIVER: 12,
    PULSE_SPEED: 0.01
  },
  WEBSOCKET: {
    URL: `ws://${window.location.hostname}/ws`,
    RECONNECT_DELAY: 2000,
    PING_INTERVAL: 30000
  }
};

// ========== ФИКСИРОВАННЫЕ ПОЗИЦИИ НА КАНВАСЕ ==========
const FIXED_POSITIONS = {
  RECEIVER: {
    x: CONFIG.CANVAS.OFFSET_X,
    y: CONFIG.CANVAS.OFFSET_Y
  },
  BEACONS: [
    {
      id: 1,
      x: CONFIG.CANVAS.OFFSET_X + (SYSTEM_CONFIG.NODE_POSITIONS.BEACON1.x * CONFIG.CANVAS.SCALE_FACTOR),
      y: CONFIG.CANVAS.OFFSET_Y + (SYSTEM_CONFIG.NODE_POSITIONS.BEACON1.y * CONFIG.CANVAS.SCALE_FACTOR)
    },
    {
      id: 2,
      x: CONFIG.CANVAS.OFFSET_X + (SYSTEM_CONFIG.NODE_POSITIONS.BEACON2.x * CONFIG.CANVAS.SCALE_FACTOR),
      y: CONFIG.CANVAS.OFFSET_Y + (SYSTEM_CONFIG.NODE_POSITIONS.BEACON2.y * CONFIG.CANVAS.SCALE_FACTOR)
    }
  ],
  SENSORS: [
    { id: 0, x: 100, y: 100 }, { id: 1, x: 900, y: 100 },
    { id: 2, x: 100, y: 600 }, { id: 3, x: 900, y: 600 },
    { id: 4, x: 300, y: 200 }, { id: 5, x: 700, y: 200 },
    { id: 6, x: 300, y: 500 }, { id: 7, x: 700, y: 500 }
  ]
};

// ========== СОСТОЯНИЕ СИСТЕМЫ ==========
let systemState = {
  receiver: { ...FIXED_POSITIONS.RECEIVER },
  beacons: FIXED_POSITIONS.BEACONS.map(beacon => ({
    ...beacon,
    active: false
  })),
  sensors: FIXED_POSITIONS.SENSORS.map(sensor => ({
    ...sensor,
    detected: false
  })),
  source: {
    x: CONFIG.CANVAS.OFFSET_X,
    y: CONFIG.CANVAS.OFFSET_Y,
    accuracy: 20,
    valid: false,
    physicalX: 0,
    physicalY: 0
  },
  activeBeaconId: -1,
  measurementActive: false,
  systemStatus: 'DISCONNECTED'
};

// ========== СИСТЕМА ИМПУЛЬСОВ ==========
let pulseSystem = {
  pulses: [],
  lastBeaconPulseTime: 0,
  lastSensorPulseTime: 0,
  BEACON_PULSE_INTERVAL: 800,
  SENSOR_PULSE_INTERVAL: 600
};

// ========== УТИЛИТЫ ДЛЯ ПРЕОБРАЗОВАНИЯ КООРДИНАТ ==========
const CoordinateUtils = {
  physicalToCanvas: function(physicalX, physicalY) {
    return {
      x: CONFIG.CANVAS.OFFSET_X + (physicalX * CONFIG.CANVAS.SCALE_FACTOR),
      y: CONFIG.CANVAS.OFFSET_Y + (physicalY * CONFIG.CANVAS.SCALE_FACTOR)
    };
  },

  canvasToPhysical: function(canvasX, canvasY) {
    return {
      x: (canvasX - CONFIG.CANVAS.OFFSET_X) / CONFIG.CANVAS.SCALE_FACTOR,
      y: (canvasY - CONFIG.CANVAS.OFFSET_Y) / CONFIG.CANVAS.SCALE_FACTOR
    };
  }
};

// ========== ФУНКЦИИ ВИЗУАЛИЗАЦИИ ==========
function createCanvasRenderer(canvasId) {
  const canvas = document.getElementById(canvasId);
  const ctx = canvas.getContext('2d');
  let width, height;

  function updateSize() {
    const container = canvas.parentElement;
    canvas.width = container.clientWidth - 30;
    canvas.height = container.clientHeight - 30;
    width = canvas.width;
    height = canvas.height;
  }

  function clear() {
    ctx.clearRect(0, 0, width, height);
    ctx.fillStyle = CONFIG.CANVAS.BACKGROUND;
    ctx.fillRect(0, 0, width, height);
  }

  function drawGrid() {
    ctx.strokeStyle = CONFIG.COLORS.GRID;
    ctx.lineWidth = 1;

    for (let x = 0; x <= width; x += CONFIG.CANVAS.GRID_SIZE) {
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, height);
      ctx.stroke();
    }

    for (let y = 0; y <= height; y += CONFIG.CANVAS.GRID_SIZE) {
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(width, y);
      ctx.stroke();
    }
  }

  function drawReceiver(receiver) {
    ctx.fillStyle = CONFIG.COLORS.RECEIVER;
    ctx.beginPath();
    ctx.moveTo(receiver.x, receiver.y - CONFIG.SIZES.RECEIVER);
    ctx.lineTo(receiver.x - CONFIG.SIZES.RECEIVER, receiver.y + CONFIG.SIZES.RECEIVER);
    ctx.lineTo(receiver.x + CONFIG.SIZES.RECEIVER, receiver.y + CONFIG.SIZES.RECEIVER);
    ctx.closePath();
    ctx.fill();

    ctx.fillStyle = 'white';
    ctx.font = '12px Courier New';
    ctx.textAlign = 'center';
    ctx.fillText('RX', receiver.x, receiver.y + 5);
  }

  function drawBeacons(beacons) {
    beacons.forEach(beacon => {
      const color = beacon.active ? CONFIG.COLORS.BEACON_ACTIVE : CONFIG.COLORS.BEACON_INACTIVE;

      ctx.fillStyle = color;
      ctx.beginPath();
      ctx.arc(beacon.x, beacon.y, CONFIG.SIZES.BEACON, 0, 2 * Math.PI);
      ctx.fill();

      if (beacon.active) {
        ctx.strokeStyle = color;
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.arc(beacon.x, beacon.y, CONFIG.SIZES.BEACON + 10, 0, 2 * Math.PI);
        ctx.stroke();
      }

      ctx.fillStyle = 'white';
      ctx.font = 'bold 14px Courier New';
      ctx.textAlign = 'center';
      ctx.fillText(`B${beacon.id}`, beacon.x, beacon.y + 5);
    });
  }

  function drawSensors(sensors) {
    sensors.forEach(sensor => {
      const color = sensor.detected ? CONFIG.COLORS.SENSOR_DETECTED : CONFIG.COLORS.SENSOR_NORMAL;

      ctx.fillStyle = color;
      ctx.beginPath();
      ctx.arc(sensor.x, sensor.y, CONFIG.SIZES.SENSOR, 0, 2 * Math.PI);
      ctx.fill();

      if (sensor.detected) {
        ctx.strokeStyle = CONFIG.COLORS.SENSOR_DETECTED;
        ctx.lineWidth = 2;
        ctx.stroke();
      }
    });
  }

  function drawSource(source) {
    if (!source.valid) return;

    ctx.fillStyle = CONFIG.COLORS.SOURCE_ACCURACY;
    ctx.beginPath();
    ctx.arc(source.x, source.y, source.accuracy, 0, 2 * Math.PI);
    ctx.fill();

    ctx.fillStyle = CONFIG.COLORS.SOURCE;
    ctx.beginPath();
    ctx.arc(source.x, source.y, CONFIG.SIZES.SOURCE, 0, 2 * Math.PI);
    ctx.fill();

    ctx.fillStyle = 'white';
    ctx.font = 'bold 12px Courier New';
    ctx.textAlign = 'center';
    ctx.fillText('SRC', source.x, source.y + 15);

    const physicalPos = CoordinateUtils.canvasToPhysical(source.x, source.y);
    ctx.fillText(
      `${physicalPos.x.toFixed(1)}cm, ${physicalPos.y.toFixed(1)}cm`,
      source.x,
      source.y + 30
    );
  }

  function drawPulse(pulse) {
    const startX = pulse.from.x;
    const startY = pulse.from.y;
    const endX = pulse.to.x;
    const endY = pulse.to.y;

    const currentX = startX + (endX - startX) * pulse.progress;
    const currentY = startY + (endY - startY) * pulse.progress;

    ctx.strokeStyle = CONFIG.COLORS.PULSE;
    ctx.lineWidth = 1.5;
    ctx.globalAlpha = pulse.alpha * 0.7;
    ctx.setLineDash([5, 3]);
    ctx.beginPath();
    ctx.moveTo(startX, startY);
    ctx.lineTo(currentX, currentY);
    ctx.stroke();

    ctx.fillStyle = CONFIG.COLORS.PULSE;
    ctx.globalAlpha = pulse.alpha;
    ctx.beginPath();
    ctx.arc(currentX, currentY, 2, 0, 2 * Math.PI);
    ctx.fill();

    ctx.globalAlpha = 1.0;
    ctx.setLineDash([]);
  }

  updateSize();
  window.addEventListener('resize', updateSize);

  return {
    clear, drawGrid, drawReceiver, drawBeacons,
    drawSensors, drawSource, drawPulse
  };
}

// ========== ОБНОВЛЕНИЕ СИСТЕМЫ ==========
function updateSystemFromData(data) {
  console.log('Received real data from ESP32:', data);

  systemState.lastUpdate = Date.now();

  if (data.source) {
    const canvasPos = CoordinateUtils.physicalToCanvas(
      data.source.x || 0,
      data.source.y || 0
    );

    systemState.source.x = canvasPos.x;
    systemState.source.y = canvasPos.y;
    systemState.source.physicalX = data.source.x || 0;
    systemState.source.physicalY = data.source.y || 0;
    systemState.source.accuracy = data.source.accuracy || 20;
    systemState.source.valid = data.source.valid || false;
  }

  systemState.activeBeaconId = data.active_beacon !== undefined ? data.active_beacon : -1;
  systemState.measurementActive = data.measuring || false;

  updateBeaconActivity();
  updateSensorDetection(data.detected_sensors || 0);
}

function updateBeaconActivity() {
  systemState.beacons.forEach(beacon => {
    beacon.active = (beacon.id === systemState.activeBeaconId);
  });
}

function updateSensorDetection(detectedCount) {
  systemState.sensors.forEach((sensor, index) => {
    sensor.detected = index < detectedCount;
  });
}

function calibrateSystem() {
  systemState.source.valid = false;
  systemState.activeBeaconId = -1;
  systemState.measurementActive = false;
  updateBeaconActivity();
  updateSensorDetection(0);
}

// ========== WEB SOCKET МЕНЕДЖЕР ==========
function createWebSocketManager() {
  let ws = null;
  let onDataCallback = null;
  let reconnectTimeout = null;

  function connect(callback) {
    onDataCallback = callback;
    console.log(`🔄 Connecting to WebSocket: ${CONFIG.WEBSOCKET.URL}`);

    try {
      ws = new WebSocket(CONFIG.WEBSOCKET.URL);
      ws.onopen = handleOpen;
      ws.onmessage = handleMessage;
      ws.onclose = handleClose;
      ws.onerror = handleError;
    } catch (error) {
      console.error('❌ WebSocket connection failed:', error);
      scheduleReconnect();
    }
  }

  function handleOpen() {
    console.log('✅ WebSocket connected to ESP32');
    updateConnectionStatus(true);
    systemState.systemStatus = 'CONNECTED';
  }

  function handleMessage(event) {
    try {
      const data = JSON.parse(event.data);
      if (onDataCallback) onDataCallback(data);
    } catch (error) {
      console.error('❌ Error parsing WebSocket message:', error, event.data);
    }
  }

  function handleClose() {
    console.log('❌ WebSocket disconnected');
    updateConnectionStatus(false);
    systemState.systemStatus = 'DISCONNECTED';
    scheduleReconnect();
  }

  function handleError(error) {
    console.error('❌ WebSocket error:', error);
  }

  function scheduleReconnect() {
    if (reconnectTimeout) clearTimeout(reconnectTimeout);
    reconnectTimeout = setTimeout(() => {
      console.log('🔄 Attempting to reconnect...');
      connect(onDataCallback);
    }, CONFIG.WEBSOCKET.RECONNECT_DELAY);
  }

  function sendCommand(command) {
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(command);
      console.log(`📤 Command sent: ${command}`);
      return true;
    } else {
      console.error(`❌ Cannot send command - WebSocket not connected: ${command}`);
      return false;
    }
  }

  return { connect, sendCommand };
}

// ========== СИСТЕМА РАДАРА ==========
function createRadarSystem(canvasId) {
  const renderer = createCanvasRenderer(canvasId);
  let animationId = null;

  function render() {
    renderer.clear();
    renderer.drawGrid();
    renderer.drawReceiver(systemState.receiver);
    renderer.drawBeacons(systemState.beacons);
    renderer.drawSensors(systemState.sensors);

    if (systemState.source.valid) {
      renderer.drawSource(systemState.source);
      generatePulses();
    }

    updateAndDrawPulses();
    animationId = requestAnimationFrame(render);
  }

  function generatePulses() {
    const currentTime = Date.now();

    const activeBeacon = systemState.beacons.find(b => b.active);
    if (activeBeacon && currentTime - pulseSystem.lastBeaconPulseTime > pulseSystem.BEACON_PULSE_INTERVAL) {
      addPulse(activeBeacon, systemState.source);
      pulseSystem.lastBeaconPulseTime = currentTime;
    }

    const detectedSensors = systemState.sensors.filter(s => s.detected);
    if (detectedSensors.length > 0 && currentTime - pulseSystem.lastSensorPulseTime > pulseSystem.SENSOR_PULSE_INTERVAL) {
      const randomSensor = detectedSensors[Math.floor(Math.random() * detectedSensors.length)];
      addPulse(systemState.source, randomSensor);
      pulseSystem.lastSensorPulseTime = currentTime;
    }
  }

  function addPulse(from, to) {
    pulseSystem.pulses.push({
      from: { x: from.x, y: from.y },
      to: { x: to.x, y: to.y },
      progress: 0,
      alpha: 1.0,
      id: Math.random().toString(36).substr(2, 9)
    });
  }

  function updateAndDrawPulses() {
    const completedPulses = [];

    pulseSystem.pulses.forEach(pulse => {
      pulse.progress += CONFIG.SIZES.PULSE_SPEED;
      pulse.alpha = 1 - pulse.progress;
      renderer.drawPulse(pulse);

      if (pulse.progress >= 1.0) {
        completedPulses.push(pulse.id);
      }
    });

    pulseSystem.pulses = pulseSystem.pulses.filter(pulse => !completedPulses.includes(pulse.id));
  }

  function resetView() {
    pulseSystem.pulses = [];
    pulseSystem.lastBeaconPulseTime = 0;
    pulseSystem.lastSensorPulseTime = 0;
  }

  function start() {
    render();
  }

  return { start, resetView };
}

// ========== ОБНОВЛЕНИЕ ИНТЕРФЕЙСА ==========
function updateUI() {
  const source = systemState.source;
  const activeBeacon = systemState.activeBeaconId;
  const detectedCount = systemState.sensors.filter(sensor => sensor.detected).length;
  const measurementStatus = systemState.measurementActive ? 'ACTIVE' : 'INACTIVE';

  document.getElementById('coordinates').textContent =
    `SOURCE: X: ${source.physicalX.toFixed(1)}cm, Y: ${source.physicalY.toFixed(1)}cm`;

  document.getElementById('systemInfo').textContent =
    `ACTIVE BEACON: ${activeBeacon !== -1 ? activeBeacon : '--'} | SENSORS: ${detectedCount} | MEAS: ${measurementStatus}`;

  updateConnectionStatus(systemState.systemStatus === 'CONNECTED');
}

function updateConnectionStatus(connected) {
  const statusEl = document.getElementById('status');
  if (connected) {
    statusEl.className = 'status connected';
    statusEl.textContent = 'SYSTEM ONLINE';
  } else {
    statusEl.className = 'status disconnected';
    statusEl.textContent = 'SYSTEM OFFLINE';
  }
}

// ========== ИНИЦИАЛИЗАЦИЯ ПРИЛОЖЕНИЯ ==========
function initializeApp() {
  const radarSystem = createRadarSystem('radarCanvas');
  const wsManager = createWebSocketManager();

  // Настройка обработчиков кнопок
  document.getElementById('startBtn').addEventListener('click', () => {
    if (wsManager.sendCommand('START')) {
      console.log('🚀 Measurement started');
    }
  });

  document.getElementById('stopBtn').addEventListener('click', () => {
    if (wsManager.sendCommand('STOP')) {
      console.log('🛑 Measurement stopped');
    }
  });

  document.getElementById('calibrateBtn').addEventListener('click', () => {
    calibrateSystem();
    radarSystem.resetView();
    if (wsManager.sendCommand('CALIBRATE')) {
      console.log('⚙️ System calibration sent');
    }
  });

  document.getElementById('resetViewBtn').addEventListener('click', () => {
    radarSystem.resetView();
    console.log('🔄 View reset');
  });

  function handleData(data) {
    updateSystemFromData(data);
    updateUI();
  }

  // ЗАПУСК РЕАЛЬНОЙ СИСТЕМЫ
  radarSystem.start();
  wsManager.connect(handleData);

  console.log('🎯 3-Beacon Positioning System - REAL MODE ACTIVATED');
  console.log('📡 Waiting for data from ESP32...');
}

// Запуск приложения
document.addEventListener('DOMContentLoaded', initializeApp);
</script>
</body>
</html>
)rawliteral";
}

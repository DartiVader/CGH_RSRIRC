#include "web_server.h"

void WebService::begin() {
    Serial.println("🌐 Starting Async Web Server (No Filesystem)...");

    // Настройка WebSocket
    webSocket.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onWebSocketEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&webSocket);

    setupRoutes();

    server.begin();
    Serial.println("✅ Async Web Server started on port 80");
    Serial.println("📡 Web interface available at: http://" + WiFi.softAPIP().toString());
}

void WebService::setupRoutes() {
    // Главная страница
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(200, "text/html", getHTMLPage());
    });

    // API для получения позиции
    server.on("/position", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String json = getJSONPosition();
        request->send(200, "application/json", json);
    });

    // Старт измерений
    server.on("/start", HTTP_GET, [this](AsyncWebServerRequest* request) {
        measurementsActive = true;
        request->send(200, "application/json", "{\"status\":\"started\"}");
        Serial.println("🚀 Measurements STARTED via web");
    });

    // Стоп измерений
    server.on("/stop", HTTP_GET, [this](AsyncWebServerRequest* request) {
        measurementsActive = false;
        request->send(200, "application/json", "{\"status\":\"stopped\"}");
        Serial.println("🛑 Measurements STOPPED via web");
    });

    // Статус системы
    server.on("/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String status = "{\"measuring\":" + String(measurementsActive ? "true" : "false") + "}";
        request->send(200, "application/json", status);
    });

    // Обработка 404
    server.onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "text/plain", "Page not found");
    });
}

String WebService::getHTMLPage() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Positioning System</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 900px;
            margin: 0 auto;
            background: white;
            border-radius: 15px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #2c3e50, #34495e);
            color: white;
            padding: 30px;
            text-align: center;
        }
        .header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        .header p {
            opacity: 0.9;
            font-size: 1.1em;
        }
        .status-panel {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 20px;
            background: #f8f9fa;
            border-bottom: 1px solid #e9ecef;
        }
        .connection-status {
            padding: 10px 20px;
            border-radius: 25px;
            font-weight: bold;
            font-size: 0.9em;
        }
        .connected {
            background: #d4edda;
            color: #155724;
            border: 2px solid #c3e6cb;
        }
        .disconnected {
            background: #f8d7da;
            color: #721c24;
            border: 2px solid #f5c6cb;
        }
        .controls {
            padding: 30px;
            text-align: center;
            background: #f8f9fa;
        }
        .btn {
            padding: 15px 30px;
            margin: 0 10px;
            font-size: 1.1em;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            transition: all 0.3s ease;
            font-weight: bold;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .btn-start {
            background: #28a745;
            color: white;
        }
        .btn-stop {
            background: #dc3545;
            color: white;
        }
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.2);
        }
        .btn:active {
            transform: translateY(0);
        }
        .position-display {
            padding: 40px;
            text-align: center;
        }
        .position-card {
            background: linear-gradient(135deg, #74b9ff, #0984e3);
            color: white;
            padding: 30px;
            border-radius: 15px;
            box-shadow: 0 10px 30px rgba(116, 185, 255, 0.3);
            margin-bottom: 20px;
        }
        .coordinates {
            font-size: 2.5em;
            font-weight: bold;
            margin-bottom: 15px;
        }
        .accuracy {
            font-size: 1.3em;
            opacity: 0.9;
            margin-bottom: 10px;
        }
        .validity {
            font-size: 1.1em;
            padding: 8px 20px;
            border-radius: 20px;
            display: inline-block;
        }
        .valid-true {
            background: rgba(46, 204, 113, 0.2);
            border: 2px solid #2ecc71;
        }
        .valid-false {
            background: rgba(231, 76, 60, 0.2);
            border: 2px solid #e74c3c;
        }
        .info-panel {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            padding: 20px;
            background: #f8f9fa;
            border-top: 1px solid #e9ecef;
        }
        .info-item {
            text-align: center;
            padding: 15px;
        }
        .info-value {
            font-size: 1.5em;
            font-weight: bold;
            color: #2c3e50;
        }
        .info-label {
            font-size: 0.9em;
            color: #7f8c8d;
            margin-top: 5px;
        }
        @media (max-width: 768px) {
            .header h1 { font-size: 2em; }
            .coordinates { font-size: 2em; }
            .btn { padding: 12px 25px; margin: 5px; }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🚀 Positioning System</h1>
            <p>Real-time Object Tracking & Localization</p>
        </div>

        <div class="status-panel">
            <div class="connection-status disconnected" id="connectionStatus">
                🔌 Disconnected
            </div>
            <div class="last-update" id="lastUpdate">
                Last update: Never
            </div>
        </div>

        <div class="controls">
            <button class="btn btn-start" onclick="sendCommand('START')">
                ▶ Start Measurements
            </button>
            <button class="btn btn-stop" onclick="sendCommand('STOP')">
                ⏹ Stop Measurements
            </button>
        </div>

        <div class="position-display">
            <div class="position-card">
                <div class="coordinates" id="coordinates">
                    X: 0.0 cm, Y: 0.0 cm
                </div>
                <div class="accuracy" id="accuracy">
                    Accuracy: 0.0 cm
                </div>
                <div class="validity valid-false" id="validity">
                    Data: Invalid
                </div>
            </div>
        </div>

        <div class="info-panel">
            <div class="info-item">
                <div class="info-value" id="clientsCount">0</div>
                <div class="info-label">Connected Clients</div>
            </div>
            <div class="info-item">
                <div class="info-value" id="measurementStatus">Stopped</div>
                <div class="info-label">Measurement Status</div>
            </div>
            <div class="info-item">
                <div class="info-value" id="updateRate">0 Hz</div>
                <div class="info-label">Update Rate</div>
            </div>
        </div>
    </div>

    <script>
        let ws = null;
        let updateCount = 0;
        let lastUpdateTime = Date.now();

        function connectWebSocket() {
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = `${protocol}//${window.location.host}/ws`;

            console.log('Connecting to WebSocket:', wsUrl);
            ws = new WebSocket(wsUrl);

            ws.onopen = function() {
                console.log('WebSocket connected');
                document.getElementById('connectionStatus').className = 'connection-status connected';
                document.getElementById('connectionStatus').textContent = '✅ Connected - Real-time Data';
                updateMeasurementStatus();
            };

            ws.onclose = function() {
                console.log('WebSocket disconnected');
                document.getElementById('connectionStatus').className = 'connection-status disconnected';
                document.getElementById('connectionStatus').textContent = '🔌 Disconnected - Reconnecting...';
                setTimeout(connectWebSocket, 2000);
            };

            ws.onerror = function(error) {
                console.error('WebSocket error:', error);
            };

            ws.onmessage = function(event) {
                const data = JSON.parse(event.data);
                updateDisplay(data);
            };
        }

        function sendCommand(command) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(command);
                if (command === 'START') {
                    setTimeout(updateMeasurementStatus, 100);
                } else if (command === 'STOP') {
                    setTimeout(updateMeasurementStatus, 100);
                }
            } else {
                alert('Not connected to the positioning system. Please wait for connection.');
            }
        }

        function updateDisplay(data) {
            // Update coordinates
            document.getElementById('coordinates').textContent =
                `X: ${data.x.toFixed(1)} cm, Y: ${data.y.toFixed(1)} cm`;

            // Update accuracy
            document.getElementById('accuracy').textContent =
                `Accuracy: ${data.accuracy.toFixed(1)} cm`;

            // Update validity
            const validityElement = document.getElementById('validity');
            if (data.valid) {
                validityElement.className = 'validity valid-true';
                validityElement.textContent = 'Data: Valid ✓';
            } else {
                validityElement.className = 'validity valid-false';
                validityElement.textContent = 'Data: Invalid ✗';
            }

            // Update timestamp
            const now = new Date(data.timestamp);
            document.getElementById('lastUpdate').textContent =
                `Last update: ${now.toLocaleTimeString()}`;

            // Calculate update rate
            updateCount++;
            const currentTime = Date.now();
            if (currentTime - lastUpdateTime >= 1000) {
                const rate = updateCount;
                document.getElementById('updateRate').textContent = `${rate} Hz`;
                updateCount = 0;
                lastUpdateTime = currentTime;
            }
        }

        async function updateMeasurementStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                document.getElementById('measurementStatus').textContent =
                    data.measuring ? 'Running' : 'Stopped';
            } catch (error) {
                console.error('Error fetching status:', error);
            }
        }

        // Initialize
        connectWebSocket();
        updateMeasurementStatus();

        // Update clients count (simulated)
        setInterval(() => {
            if (ws && ws.readyState === WebSocket.OPEN) {
                document.getElementById('clientsCount').textContent = '1';
            } else {
                document.getElementById('clientsCount').textContent = '0';
            }
        }, 5000);
    </script>
</body>
</html>
)rawliteral";
    return html;
}

void WebService::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch(type) {
        case WS_EVT_CONNECT:
            Serial.println("🔗 WebSocket client connected");
            // Отправляем текущую позицию при подключении
            client->text(getJSONPosition());
            break;

        case WS_EVT_DISCONNECT:
            Serial.println("🔌 WebSocket client disconnected");
            break;

        case WS_EVT_DATA:
            if (len > 0) {
                String message = String((char*)data).substring(0, len);
                Serial.println("📨 WebSocket command: " + message);

                if (message == "START") {
                    measurementsActive = true;
                    client->text("{\"type\":\"status\",\"message\":\"Measurements started\"}");
                    Serial.println("🚀 Measurements STARTED via WebSocket");
                } else if (message == "STOP") {
                    measurementsActive = false;
                    client->text("{\"type\":\"status\",\"message\":\"Measurements stopped\"}");
                    Serial.println("🛑 Measurements STOPPED via WebSocket");
                }
            }
            break;

        case WS_EVT_ERROR:
            Serial.println("❌ WebSocket error");
            break;
    }
}

String WebService::getJSONPosition() {
    StaticJsonDocument<200> doc;
    doc["x"] = currentPosition.x;
    doc["y"] = currentPosition.y;
    doc["accuracy"] = currentPosition.accuracy;
    doc["valid"] = currentPosition.valid;
    doc["timestamp"] = currentPosition.timestamp;

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

void WebService::updatePosition(const PositionData& newPosition) {
    currentPosition = newPosition;
    broadcastPosition();
}

void WebService::broadcastPosition() {
    String json = getJSONPosition();
    webSocket.textAll(json);
}

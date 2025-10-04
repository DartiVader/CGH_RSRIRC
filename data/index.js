const CONFIG = {
  CANVAS: {
    BACKGROUND: '#050a05',
    GRID_SIZE: 50
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
    URL: 'ws://192.168.4.1/ws',
    RECONNECT_DELAY: 2000
  }
};

const FIXED_POSITIONS = {
  RECEIVER: { x: 500, y: 350 },
  BEACONS: [
    { id: 1, x: 200, y: 300 },
    { id: 2, x: 800, y: 300 }
  ],
  SENSORS: [
    { id: 0, x: 100, y: 100 }, { id: 1, x: 900, y: 100 },
    { id: 2, x: 100, y: 600 }, { id: 3, x: 900, y: 600 },
    { id: 4, x: 300, y: 200 }, { id: 5, x: 700, y: 200 },
    { id: 6, x: 300, y: 500 }, { id: 7, x: 700, y: 500 }
  ]
};

let systemState = {
  receiver: { ...FIXED_POSITIONS.RECEIVER },
  beacons: FIXED_POSITIONS.BEACONS.map(beacon => ({ ...beacon, active: false })),
  sensors: FIXED_POSITIONS.SENSORS.map(sensor => ({ ...sensor, detected: false })),
  source: { x: 500, y: 350, accuracy: 20, valid: false },
  activeBeaconId: null
};

let pulseSystem = {
  pulses: [],
  lastBeaconPulseTime: 0,
  lastSensorPulseTime: 0,
  BEACON_PULSE_INTERVAL: 800,
  SENSOR_PULSE_INTERVAL: 600
};

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
    clear, drawGrid, drawReceiver, drawBeacons, drawSensors, drawSource, drawPulse
  };
}

function updateSystemFromData(data) {
  if (data.source) {
    systemState.source.x = data.source.x || 500;
    systemState.source.y = data.source.y || 350;
    systemState.source.accuracy = data.source.accuracy || 20;
    systemState.source.valid = true;
  }

  systemState.activeBeaconId = data.active_beacon !== undefined ? data.active_beacon : null;
  updateBeaconActivity();

  updateSensorDetection(data.detected_sensors || []);
}

function updateBeaconActivity() {
  systemState.beacons.forEach(beacon => {
    beacon.active = (beacon.id === systemState.activeBeaconId);
  });
}

function updateSensorDetection(detectedSensorIds) {
  systemState.sensors.forEach(sensor => {
    sensor.detected = false;
  });

  detectedSensorIds.forEach(sensorId => {
    const sensor = systemState.sensors.find(s => s.id === sensorId);
    if (sensor) sensor.detected = true;
  });
}

function calibrateSystem() {
  systemState.source.valid = false;
  systemState.activeBeaconId = null;
  updateBeaconActivity();
  updateSensorDetection([]);
}

function createWebSocketManager() {
  let ws = null;
  let onDataCallback = null;
  let reconnectTimeout = null;

  function connect(callback) {
    onDataCallback = callback;

    try {
      ws = new WebSocket(CONFIG.WEBSOCKET.URL);
      ws.onopen = handleOpen;
      ws.onmessage = handleMessage;
      ws.onclose = handleClose;
      ws.onerror = handleError;
    } catch (error) {
      console.error('WebSocket connection failed:', error);
      scheduleReconnect();
    }
  }

  function handleOpen() {
    console.log('+ WebSocket connected');
    updateConnectionStatus(true);
  }

  function handleMessage(event) {
    try {
      const data = JSON.parse(event.data);
      if (onDataCallback) onDataCallback(data);
    } catch (error) {
      console.error('Error parsing WebSocket message:', error);
    }
  }

  function handleClose() {
    console.log('- WebSocket disconnected');
    updateConnectionStatus(false);
    scheduleReconnect();
  }

  function handleError(error) {
    console.error('WebSocket error:', error);
  }

  function scheduleReconnect() {
    if (reconnectTimeout) clearTimeout(reconnectTimeout);
    reconnectTimeout = setTimeout(() => {
      console.log('Attempting to reconnect...');
      connect(onDataCallback);
    }, CONFIG.WEBSOCKET.RECONNECT_DELAY);
  }

  return { connect };
}

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
      id: Math.random().toString(36).substr(2, 9) // Уникальный ID для импульса
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

function updateUI() {
  const source = systemState.source;
  const activeBeacon = systemState.activeBeaconId;
  const detectedCount = systemState.sensors.filter(sensor => sensor.detected).length;

  document.getElementById('coordinates').textContent =
    `Source: X: ${source.x.toFixed(2)}, Y: ${source.y.toFixed(2)}`;

  document.getElementById('systemInfo').textContent =
    `Active Beacon: ${activeBeacon || '-'} | Detected Sensors: ${detectedCount}`;
}

function updateConnectionStatus(connected) {
  const statusEl = document.getElementById('status');
  if (connected) {
    statusEl.className = 'status connected';
    statusEl.textContent = 'Connected';
  } else {
    statusEl.className = 'status disconnected';
    statusEl.textContent = 'Disconnected';
  }
}

// ========== ТЕСТОВЫЕ ДАННЫЕ ==========
function startAutoTest() {
  let angle = 0;
  let testFrameCount = 0;

  setInterval(() => {
    testFrameCount++;


    const sourceX = 500 + 200 * Math.cos(angle);
    const sourceY = 350 + 200 * Math.sin(angle);

    const distToBeacon1 = Math.sqrt(Math.pow(sourceX - 200, 2) + Math.pow(sourceY - 300, 2));
    const distToBeacon2 = Math.sqrt(Math.pow(sourceX - 800, 2) + Math.pow(sourceY - 300, 2));

    let activeBeacon;
    if (distToBeacon1 < 400) {
      activeBeacon = 1;
    } else if (distToBeacon2 < 400) {
      activeBeacon = 2;
    } else {
      activeBeacon = distToBeacon1 < distToBeacon2 ? 1 : 2;
    }

    const detectedSensors = [];
    systemState.sensors.forEach(sensor => {
      const dist = Math.sqrt(Math.pow(sourceX - sensor.x, 2) + Math.pow(sourceY - sensor.y, 2));
      if (dist < 300 && Math.random() > 0.7) {
        detectedSensors.push(sensor.id);
      }
    });

    const testData = {
      source: {
        x: sourceX,
        y: sourceY,
        accuracy: 15 + Math.random() * 10
      },
      active_beacon: activeBeacon,
      detected_sensors: detectedSensors
    };

    updateSystemFromData(testData);
    updateUI();
    angle += 0.02;

  }, 150);
}
// ========== КОНЕЦ ТЕСТОВЫХ ДАННЫХ ==========

function initializeApp() {
  const radarSystem = createRadarSystem('radarCanvas');
  const wsManager = createWebSocketManager();

  document.getElementById('calibrateBtn').addEventListener('click', () => {
    calibrateSystem();
    radarSystem.resetView();
  });

  document.getElementById('resetViewBtn').addEventListener('click', () => {
    radarSystem.resetView();
  });

  function handleData(data) {
    updateSystemFromData(data);
    updateUI();
  }

  radarSystem.start();

  // ========== ЗАПУСК ТЕСТОВОГО РЕЖИМА ==========
  startAutoTest();
  console.log('Тестовый режим активен');
  // ========== КОНЕЦ ТЕСТОВОГО РЕЖИМА ==========

  // ========== ДЛЯ ПОДКЛЮЧЕНИЯ ESP32 РАСКОММЕНТИРОВАТЬ СЛЕДУЮЩУЮ СТРОКУ ==========
  // wsManager.connect(handleData);

  console.log('Positioning System started');
}

document.addEventListener('DOMContentLoaded', initializeApp);
import WebSocketManager from './WebSocketManager.js';
import RadarSystem from './RadarSystem.js';
import SystemObjects from './SystemObjects.js';

class App {
  constructor() {
    this.objects = new SystemObjects();
    this.radar = new RadarSystem('radarCanvas', this.objects);
    this.wsManager = new WebSocketManager();

    this.init();
  }

  init() {
    this.setupEventListeners();
    this.radar.start();
    this.wsManager.connect(this.handleData.bind(this));
    console.log('🚀 Positioning System started');
  }

  setupEventListeners() {
    document.getElementById('calibrateBtn').addEventListener('click', () => {
      this.calibrateSystem();
    });

    document.getElementById('resetViewBtn').addEventListener('click', () => {
      this.radar.resetView();
    });
  }

  handleData(data) {
    try {
      this.objects.updateFromData(data);
      this.updateUI();
      this.updateConnectionStatus(true);
    } catch (error) {
      console.error('Error processing data:', error);
      this.updateConnectionStatus(false);
    }
  }

  updateUI() {
    const source = this.objects.getSource();
    const activeBeacon = this.objects.getActiveBeacon();
    const detectedCount = this.objects.getDetectedSensorsCount();

    document.getElementById('coordinates').textContent =
      `Source: X: ${source.x.toFixed(2)}, Y: ${source.y.toFixed(2)}`;

    document.getElementById('systemInfo').textContent =
      `Active Beacon: ${activeBeacon || '-'} | Detected Sensors: ${detectedCount}`;
  }

  updateConnectionStatus(connected) {
    const statusEl = document.getElementById('status');
    if (connected) {
      statusEl.className = 'status connected';
      statusEl.textContent = 'Connected';
    } else {
      statusEl.className = 'status disconnected';
      statusEl.textContent = 'Disconnected';
    }
  }

  calibrateSystem() {
    console.log('🔧 Calibrating system...');
    this.objects.calibrate();
    this.radar.resetView();
  }
}

export default App;
const CONFIG = {
  CANVAS: {
    BACKGROUND: '#050a05',
    GRID_SIZE: 50
  },
  COLORS: {
    SOURCE: '#ff3232',
    SOURCE_ACCURACY: 'rgba(255, 50, 50, 0.2)',
    BEACON_ACTIVE: '#64ffa5',
    BEACON_INACTIVE: '#1a3a1a',
    SENSOR_DETECTED: '#b7fe65',
    SENSOR_NORMAL: '#2a4a2a',
    RECEIVER: '#64ffa5',
    GRID: '#1a3a1a',
    PULSE: '#64ffa5'
  },

  SIZES: {
    SOURCE: 10,
    BEACON: 18,
    SENSOR: 9,
    RECEIVER: 14,
    PULSE_SPEED: 0.04
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

export { CONFIG, FIXED_POSITIONS };
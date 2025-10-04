import { FIXED_POSITIONS } from './Config.js';

class SystemObjects {
  constructor() {
    this.receiver = { ...FIXED_POSITIONS.RECEIVER };
    this.beacons = FIXED_POSITIONS.BEACONS.map(beacon => ({
      ...beacon,
      active: false
    }));
    this.sensors = FIXED_POSITIONS.SENSORS.map(sensor => ({
      ...sensor,
      detected: false
    }));
    this.source = { x: 500, y: 350, accuracy: 20, valid: false };
    this.activeBeaconId = null;
  }

  updateFromData(data) {
    if (data.source) {
      this.source.x = data.source.x;
      this.source.y = data.source.y;
      this.source.accuracy = data.source.accuracy || 20;
      this.source.valid = true;
    }

    if (data.active_beacon !== undefined) {
      this.activeBeaconId = data.active_beacon;
      this.updateBeaconActivity();
    }

    if (data.detected_sensors) {
      this.updateSensorDetection(data.detected_sensors);
    }
  }

  updateBeaconActivity() {
    this.beacons.forEach(beacon => {
      beacon.active = (beacon.id === this.activeBeaconId);
    });
  }

  updateSensorDetection(detectedSensorIds) {
    this.sensors.forEach(sensor => {
      sensor.detected = false;
    });

    detectedSensorIds.forEach(sensorId => {
      const sensor = this.sensors.find(s => s.id === sensorId);
      if (sensor) {
        sensor.detected = true;
      }
    });
  }

  getSource() {
    return this.source;
  }

  getActiveBeacon() {
    return this.activeBeaconId;
  }

  getDetectedSensorsCount() {
    return this.sensors.filter(sensor => sensor.detected).length;
  }

  getAllObjects() {
    return {
      receiver: this.receiver,
      beacons: this.beacons,
      sensors: this.sensors,
      source: this.source
    };
  }

  calibrate() {
    this.source.valid = false;
    this.activeBeaconId = null;
    this.updateBeaconActivity();
    this.updateSensorDetection([]);
  }
}

export default SystemObjects;
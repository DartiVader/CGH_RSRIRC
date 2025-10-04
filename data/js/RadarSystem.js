// js/RadarSystem.js
import { CONFIG } from './Config.js';
import CanvasRenderer from './CanvasRender.js';

class RadarSystem {
  constructor(canvasId, systemObjects) {
    this.canvas = document.getElementById(canvasId);
    this.renderer = new CanvasRenderer(this.canvas);
    this.objects = systemObjects;
    this.pulses = [];
    this.animationId = null;
  }

  start() {
    this.render();
  }

  stop() {
    if (this.animationId) {
      cancelAnimationFrame(this.animationId);
    }
  }

  render() {
    this.renderer.clear();
    this.renderer.drawGrid();

    const systemObjects = this.objects.getAllObjects();
    this.renderer.drawReceiver(systemObjects.receiver);
    this.renderer.drawBeacons(systemObjects.beacons);
    this.renderer.drawSensors(systemObjects.sensors);

    if (systemObjects.source.valid) {
      this.renderer.drawSource(systemObjects.source);
      this.drawConnections(systemObjects);
    }

    this.updatePulses();
    this.animationId = requestAnimationFrame(() => this.render());
  }

  drawConnections(systemObjects) {
    const activeBeacon = systemObjects.beacons.find(b => b.active);

    if (activeBeacon) {
      this.addPulse(activeBeacon, systemObjects.source);
    }

    systemObjects.sensors.forEach(sensor => {
      if (sensor.detected) {
        this.addPulse(systemObjects.source, sensor);
      }
    });

    this.pulses.forEach(pulse => {
      this.renderer.drawPulse(pulse);
    });
  }

  addPulse(from, to) {
    const pulseExists = this.pulses.some(p =>
      p.from.x === from.x && p.from.y === from.y &&
      p.to.x === to.x && p.to.y === to.y
    );

    if (!pulseExists) {
      this.pulses.push({
        from: { x: from.x, y: from.y },
        to: { x: to.x, y: to.y },
        progress: 0,
        alpha: 1.0
      });
    }
  }

  updatePulses() {
    this.pulses.forEach(pulse => {
      pulse.progress += CONFIG.SIZES.PULSE_SPEED;
      pulse.alpha = 1 - pulse.progress;
    });

    this.pulses = this.pulses.filter(pulse => pulse.progress < 1.0);
  }

  resetView() {
    this.pulses = [];
  }
}

export default RadarSystem;
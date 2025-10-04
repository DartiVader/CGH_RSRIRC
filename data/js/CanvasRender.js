import { CONFIG } from './Config.js';

class CanvasRenderer {
  constructor(canvas) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    this.updateSize(); // Вызываем при создании
  }

  updateSize() {
    // Получаем реальные размеры контейнера
    const container = this.canvas.parentElement;
    this.canvas.width = container.clientWidth - 30; // Учитываем padding
    this.canvas.height = container.clientHeight - 30;
    this.width = this.canvas.width;
    this.height = this.canvas.height;
  }
  clear() {
    this.ctx.clearRect(0, 0, this.width, this.height);
    this.ctx.fillStyle = CONFIG.CANVAS.BACKGROUND;
    this.ctx.fillRect(0, 0, this.width, this.height);
  }

  drawGrid() {
    this.ctx.strokeStyle = CONFIG.COLORS.GRID;
    this.ctx.lineWidth = 1;

    for (let x = 0; x <= this.width; x += CONFIG.CANVAS.GRID_SIZE) {
      this.ctx.beginPath();
      this.ctx.moveTo(x, 0);
      this.ctx.lineTo(x, this.height);
      this.ctx.stroke();
    }

    for (let y = 0; y <= this.height; y += CONFIG.CANVAS.GRID_SIZE) {
      this.ctx.beginPath();
      this.ctx.moveTo(0, y);
      this.ctx.lineTo(this.width, y);
      this.ctx.stroke();
    }
  }

  drawReceiver(receiver) {
    this.ctx.fillStyle = CONFIG.COLORS.RECEIVER;
    this.ctx.beginPath();
    this.ctx.moveTo(receiver.x, receiver.y - CONFIG.SIZES.RECEIVER);
    this.ctx.lineTo(receiver.x - CONFIG.SIZES.RECEIVER, receiver.y + CONFIG.SIZES.RECEIVER);
    this.ctx.lineTo(receiver.x + CONFIG.SIZES.RECEIVER, receiver.y + CONFIG.SIZES.RECEIVER);
    this.ctx.closePath();
    this.ctx.fill();

    this.ctx.fillStyle = 'white';
    this.ctx.font = '12px Arial';
    this.ctx.textAlign = 'center';
    this.ctx.fillText('RX', receiver.x, receiver.y + 5);
  }

  drawBeacons(beacons) {
    beacons.forEach(beacon => {
      const color = beacon.active ? CONFIG.COLORS.BEACON_ACTIVE : CONFIG.COLORS.BEACON_INACTIVE;

      this.ctx.fillStyle = color;
      this.ctx.beginPath();
      this.ctx.arc(beacon.x, beacon.y, CONFIG.SIZES.BEACON, 0, 2 * Math.PI);
      this.ctx.fill();

      if (beacon.active) {
        this.ctx.strokeStyle = color;
        this.ctx.lineWidth = 2;
        this.ctx.beginPath();
        this.ctx.arc(beacon.x, beacon.y, CONFIG.SIZES.BEACON + 10, 0, 2 * Math.PI);
        this.ctx.stroke();
      }

      this.ctx.fillStyle = 'white';
      this.ctx.font = 'bold 14px Arial';
      this.ctx.textAlign = 'center';
      this.ctx.fillText(`B${beacon.id}`, beacon.x, beacon.y + 5);
    });
  }

  drawSensors(sensors) {
    sensors.forEach(sensor => {
      const color = sensor.detected ? CONFIG.COLORS.SENSOR_DETECTED : CONFIG.COLORS.SENSOR_NORMAL;

      this.ctx.fillStyle = color;
      this.ctx.beginPath();
      this.ctx.arc(sensor.x, sensor.y, CONFIG.SIZES.SENSOR, 0, 2 * Math.PI);
      this.ctx.fill();

      if (sensor.detected) {
        this.ctx.strokeStyle = CONFIG.COLORS.SENSOR_DETECTED;
        this.ctx.lineWidth = 2;
        this.ctx.stroke();
      }
    });
  }

  drawSource(source) {
    this.ctx.fillStyle = CONFIG.COLORS.SOURCE_ACCURACY;
    this.ctx.beginPath();
    this.ctx.arc(source.x, source.y, source.accuracy, 0, 2 * Math.PI);
    this.ctx.fill();

    this.ctx.fillStyle = CONFIG.COLORS.SOURCE;
    this.ctx.beginPath();
    this.ctx.arc(source.x, source.y, CONFIG.SIZES.SOURCE, 0, 2 * Math.PI);
    this.ctx.fill();

    this.ctx.fillStyle = 'white';
    this.ctx.font = 'bold 12px Arial';
    this.ctx.textAlign = 'center';
    this.ctx.fillText('SRC', source.x, source.y + 15);
  }

  drawPulse(pulse) {
    const startX = pulse.from.x;
    const startY = pulse.from.y;
    const endX = pulse.to.x;
    const endY = pulse.to.y;

    const currentX = startX + (endX - startX) * pulse.progress;
    const currentY = startY + (endY - startY) * pulse.progress;

    this.ctx.strokeStyle = CONFIG.COLORS.PULSE;
    this.ctx.lineWidth = 2;
    this.ctx.globalAlpha = pulse.alpha;
    this.ctx.beginPath();
    this.ctx.moveTo(startX, startY);
    this.ctx.lineTo(currentX, currentY);
    this.ctx.stroke();

    this.ctx.fillStyle = CONFIG.COLORS.PULSE;
    this.ctx.beginPath();
    this.ctx.arc(currentX, currentY, 3, 0, 2 * Math.PI);
    this.ctx.fill();

    this.ctx.globalAlpha = 1.0;
  }

  reset() {
    // Сброс рендерера
  }
}

export default CanvasRenderer;
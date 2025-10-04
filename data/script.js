class PositioningClient {
    constructor() {
        this.ws = null;
        this.isConnected = false;
        this.connectedClients = 0;
        this.init();
    }

    init() {
        this.connectWebSocket();
        this.startStatusPolling();
    }

    connectWebSocket() {
        const wsUrl = `ws://${window.location.hostname}/ws`;
        this.ws = new WebSocket(wsUrl);

        this.ws.onopen = () => {
            console.log('✅ WebSocket connected');
            this.isConnected = true;
            this.updateConnectionStatus(true);
        };

        this.ws.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                this.handleMessage(data);
            } catch (e) {
                console.error('❌ Parse error:', e);
            }
        };

        this.ws.onclose = () => {
            console.log('❌ WebSocket disconnected');
            this.isConnected = false;
            this.updateConnectionStatus(false);
            setTimeout(() => this.connectWebSocket(), 3000);
        };

        this.ws.onerror = (error) => {
            console.error('❌ WebSocket error:', error);
        };
    }

    handleMessage(data) {
        switch(data.type) {
            case 'position':
                this.updatePosition(data);
                break;
            case 'status':
                console.log('System status:', data.message);
                break;
            default:
                console.log('Unknown message type:', data);
        }
    }

    updatePosition(data) {
        // Обновление координат
        document.getElementById('coordinates').textContent = 
            `X: ${data.x.toFixed(1)}, Y: ${data.y.toFixed(1)}`;
        document.getElementById('accuracy').textContent = 
            `± ${data.accuracy.toFixed(1)} cm`;

        // Обновление позиции на карте
        this.updateMapPosition(data.x, data.y);
    }

    updateMapPosition(x, y) {
        const object = document.getElementById('object');
        const map = document.getElementById('map');
        
        // Масштабирование координат (400x300 см -> размеры карты)
        const scaleX = map.offsetWidth / 400;
        const scaleY = map.offsetHeight / 300;
        
        const posX = x * scaleX;
        const posY = y * scaleY;
        
        object.style.display = 'block';
        object.style.left = `${posX}px`;
        object.style.top = `${posY}px`;
    }

    updateConnectionStatus(connected) {
        const status = document.getElementById('ws-status');
        status.textContent = connected ? 'Connected' : 'Disconnected';
        status.className = connected ? 'connected' : 'disconnected';
    }

    updateClientsCount(count) {
        this.connectedClients = count;
        document.getElementById('clients-count').textContent = count;
    }

    async startStatusPolling() {
        setInterval(async () => {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                this.updateClientsCount(data.clients);
            } catch (error) {
                console.error('Status polling error:', error);
            }
        }, 2000);
    }

    sendCommand(command) {
        if (this.ws && this.isConnected) {
            this.ws.send(command);
            console.log(`📤 Command sent: ${command}`);
            
            // Обновление состояния кнопок
            const startBtn = document.getElementById('btn-start');
            const stopBtn = document.getElementById('btn-stop');
            
            if (command === 'START') {
                startBtn.disabled = true;
                stopBtn.disabled = false;
            } else if (command === 'STOP') {
                startBtn.disabled = false;
                stopBtn.disabled = true;
            }
        } else {
            alert('WebSocket not connected. Please wait...');
        }
    }
}

// Глобальные функции для кнопок
function sendCommand(command) {
    if (window.positioningClient) {
        window.positioningClient.sendCommand(command);
    }
}

// Инициализация при загрузке
window.onload = () => {
    window.positioningClient = new PositioningClient();
};
class PositioningClient {
    constructor() {
        this.ws = null;
        this.isConnected = false;
        this.init();
    }

    init() {
        this.connectWebSocket();
    }

    connectWebSocket() {
        const wsUrl = `ws://${window.location.hostname}/ws`;
        this.ws = new WebSocket(wsUrl);

        this.ws.onopen = () => {
            console.log('✅ WebSocket connected');
            this.isConnected = true;
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
            setTimeout(() => this.connectWebSocket(), 3000);
        };

        this.ws.onerror = (error) => {
            console.error('❌ WebSocket error:', error);
        };
    }

    handleMessage(data) {
        if (data.type === 'status') {
            console.log('System status:', data.message);
        } else {
            this.updatePosition(data);
        }
    }

    updatePosition(data) {
        document.getElementById('coordinates').textContent = 
            `X: ${data.x.toFixed(1)}, Y: ${data.y.toFixed(1)}`;
        document.getElementById('accuracy').textContent = 
            `± ${data.accuracy.toFixed(1)} cm`;

        this.updateMapPosition(data.x, data.y);
    }

    updateMapPosition(x, y) {
        const object = document.getElementById('object');
        const map = document.getElementById('map');
        
        const scaleX = map.offsetWidth / 400;
        const scaleY = map.offsetHeight / 300;
        
        const posX = x * scaleX;
        const posY = y * scaleY;
        
        object.style.display = 'block';
        object.style.left = `${posX}px`;
        object.style.top = `${posY}px`;
    }

    sendCommand(command) {
        if (this.ws && this.isConnected) {
            this.ws.send(command);
            console.log(`📤 Command sent: ${command}`);
            
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

function sendCommand(command) {
    if (window.positioningClient) {
        window.positioningClient.sendCommand(command);
    }
}

window.onload = () => {
    window.positioningClient = new PositioningClient();
};
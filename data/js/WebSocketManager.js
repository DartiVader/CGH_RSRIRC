import { CONFIG } from './Config.js';

class WebSocketManager {
    constructor() {
        this.ws = null;
        this.onDataCallback = null;
        this.reconnectTimeout = null;
    }

    connect(onDataCallback) {
        this.onDataCallback = onDataCallback;

        try {
            this.ws = new WebSocket(CONFIG.WEBSOCKET.URL);
            this.ws.onopen = this.handleOpen.bind(this);
            this.ws.onmessage = this.handleMessage.bind(this);
            this.ws.onclose = this.handleClose.bind(this);
            this.ws.onerror = this.handleError.bind(this);
        } catch (error) {
            console.error('WebSocket connection failed:', error);
            this.scheduleReconnect();
        }
    }

    handleOpen() {
        console.log('✅ WebSocket connected');
    }

    handleMessage(event) {
        try {
            const data = JSON.parse(event.data);
            if (this.onDataCallback) {
                this.onDataCallback(data);
            }
        } catch (error) {
            console.error('Error parsing WebSocket message:', error);
        }
    }

    handleClose() {
        console.log('- WebSocket disconnected');
        this.scheduleReconnect();
    }

    handleError(error) {
        console.error('WebSocket error:', error);
    }

    scheduleReconnect() {
        if (this.reconnectTimeout) {
            clearTimeout(this.reconnectTimeout);
        }
        this.reconnectTimeout = setTimeout(() => {
            console.log('Attempting to reconnect...');
            this.connect(this.onDataCallback);
        }, CONFIG.WEBSOCKET.RECONNECT_DELAY);
    }

    send(data) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(data));
        }
    }

    disconnect() {
        if (this.reconnectTimeout) {
            clearTimeout(this.reconnectTimeout);
        }
        if (this.ws) {
            this.ws.close();
        }
    }
}

export default WebSocketManager;
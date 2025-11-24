const express = require('express');
const http = require('http');
const WebSocket = require('ws');

const app = express();
const server = http.createServer(app);

// Attach WebSocket server to the same HTTP server
const wss = new WebSocket.Server({ server });

// Store connected clients (frontend)
let frontendClients = [];

// Latest ECG value
let ecgValue = 0;

// WebSocket connections
wss.on('connection', (ws, req) => {
    console.log("📡 WebSocket connected:", req.socket.remoteAddress);

    ws.isESP32 = false; // mark if this is ESP32

    ws.on('message', (msg) => {
        if (Buffer.isBuffer(msg) && msg.length === 4) {
            // Treat as ESP32
            ws.isESP32 = true;
            ecgValue = msg.readInt32LE(0);
            console.log("📥 ECG from ESP32:", ecgValue);

            // Broadcast to frontend clients
            frontendClients.forEach(client => {
                if (client.readyState === WebSocket.OPEN) {
                    client.send(ecgValue.toString());
                }
            });

        } else {
            // Treat as frontend client message (optional)
            console.log("📥 Message from frontend:", msg.toString());
        }
    });

    // If this connection is a frontend client, push to array immediately
    ws.send("No Data From ECG"); // welcome
    frontendClients.push(ws);

    ws.on('close', () => {
        frontendClients = frontendClients.filter(c => c !== ws);
        console.log("Client disconnected");
    });
});


// Serve a simple route for testing
app.get('/', (req, res) => {
    res.send("WebSocket interceptor server running." + ecgValue);
});

// Start server
const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
    console.log(`🚀 Server running on port ${PORT}`);
});

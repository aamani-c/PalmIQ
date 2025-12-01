const express = require('express');
const { WebSocketServer } = require('ws');
const cors = require('cors');

const app = express();
const PORT = 3000;

// Enable CORS for React app
app.use(cors());
app.use(express.json());

// Store the latest sensor data
let latestSensorData = {
  heart: null,
  spo2: null,
  temp_c: null,
  timestamp: null
};

// Create HTTP server
const server = app.listen(PORT, '0.0.0.0', () => {
  console.log(`✅ Server running on http://0.0.0.0:${PORT}`);
  console.log(`📡 WebSocket server ready for ESP32 connections`);
});

// Create WebSocket server on the same HTTP server
const wss = new WebSocketServer({ server });

// Handle WebSocket connections
wss.on('connection', (ws, req) => {
  const clientIP = req.socket.remoteAddress;
  console.log(`\n🔌 WebSocket client connected from ${clientIP}`);

  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message.toString());
      console.log('\n📥 Received sensor data:', data);
      
      // Update latest sensor data
      latestSensorData = {
        heart: data.heart,
        spo2: data.spo2,
        temp_c: data.temp_c,
        timestamp: new Date().toISOString()
      };
      
      console.log('💾 Updated sensor data:', latestSensorData);
      
      // Send acknowledgment back to ESP32
      ws.send(JSON.stringify({ status: 'ok', received: true }));
    } catch (error) {
      console.error('❌ Error parsing message:', error);
    }
  });

  ws.on('close', () => {
    console.log(`\n🔌 WebSocket client disconnected from ${clientIP}`);
  });

  ws.on('error', (error) => {
    console.error('❌ WebSocket error:', error);
  });

  // Send welcome message
  ws.send(JSON.stringify({ message: 'Connected to health monitoring server' }));
});

// REST API endpoint for React app to fetch data
app.get('/api/sensors', (req, res) => {
  console.log('📤 GET /api/sensors - Sending data to React app');
  res.json(latestSensorData);
});

// Health check endpoint
app.get('/health', (req, res) => {
  res.json({ status: 'ok', uptime: process.uptime() });
});

console.log('\n📋 Available endpoints:');
console.log('   GET  /api/sensors - Get latest sensor data (for React)');
console.log('   GET  /health - Server health check');
console.log('   WS   / - WebSocket endpoint (for ESP32)');
console.log('\n⚠️  Make sure to update WS_HOST in ESP32 code to your server IP');
console.log('   Example: const char* WS_HOST = "192.168.1.100";\n');
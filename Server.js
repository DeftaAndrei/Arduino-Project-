const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const mqtt = require('mqtt');
const cors = require('cors');
const bodyParser = require('body-parser');
const cookieParser = require('cookie-parser');
const mongoSanitize = require('express-mongo-sanitize');
const helmet = require('helmet');
const xss = require('xss-clean');
const rateLimit = require('express-rate-limit');
const hpp = require('hpp');
require('dotenv').config();
// Import middlewares și utilități
const errorHandler = require('./middleware/error');
const connectDB = require('./config/db');
const { startJsonSystem, stopJsonSystem } = require('./controllers/jsonDataController');
const { setMqttClient, updateRelayState } = require('./controllers/relayController');
222
// Conectare la baza de date MongoDB
connectDB();
// Inițializează sistemul de citire JSON
startJsonSystem();
const app = express();
// Middleware pentru securitate și performanță
app.use(cors({
origin: process.env.NODE_ENV === 'production' ? 'https://yourdomain.com' : 'http://localhost:3000',
credentials: true
}));
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(cookieParser());
// Middleware pentru securitate
app.use(mongoSanitize()); // Previne injecția NoSQL
app.use(helmet()); // Setează headere HTTP de securitate
app.use(xss()); // Previne Cross-Site Scripting (XSS)
// Limitarea ratei de request-uri pentru API
const limiter = rateLimit({
windowMs: 10 * 60 * 1000, // 10 minute
max: 100 // 100 request-uri per IP în fereastra de timp
});
app.use('/api', limiter);
// Previne HTTP Parameter Pollution
app.use(hpp());
const server = http.createServer(app);
const io = socketIo(server, {
cors: {
origin: "*", // Permite conexiuni de oriunde în mediul de dezvoltare
methods: ["GET", "POST"]
}
});
// Configurare MQTT - Conectare la broker-ul local
const MQTT_BROKER_URL = process.env.MQTT_BROKER_URL || 'mqtt://192.168.1.137:1883';
223
console.log('🔌 Conectare la MQTT Broker:', MQTT_BROKER_URL);
const mqttClient = mqtt.connect(MQTT_BROKER_URL);
mqttClient.on('connect', () => {
console.log('✅ Connected to MQTT broker la:', MQTT_BROKER_URL);
// Setează clientul MQTT în relayController
setMqttClient(mqttClient);
// Abonare la topicuri relevante
mqttClient.subscribe('esp/+/data');
mqttClient.subscribe('esp/+/status');
mqttClient.subscribe('esp/relay/status');
mqttClient.subscribe('esp/relay/heartbeat');
mqttClient.subscribe('esp/mega/status');
// Abonare la topicul pentru senzori
mqttClient.subscribe('automatizare/esp32/senzori');
mqttClient.subscribe('automatizare/+/+');
mqttClient.subscribe('$SYS/#'); // Pentru monitorizare sistem
console.log('📡 Subscribed to MQTT topics including:', [
'esp/+/data', 'esp/+/status', 'automatizare/esp32/senzori'
]);
});
// Handler pentru erori MQTT
mqttClient.on('error', (error) => {
console.error('❌ MQTT Error:', error.message);
});
// Handler pentru deconectare
mqttClient.on('offline', () => {
console.log('⚠️ MQTT Client offline - broker indisponibil');
});
// Handler pentru reconectare
mqttClient.on('reconnect', () => {
console.log('🔄 Încercare reconectare la MQTT broker...');
});
// Handler pentru închidere conexiune
224
mqttClient.on('close', () => {
console.log('🔌 Conexiune MQTT închisă');
});
// Monitorizare mesaje MQTT în timp real
mqttClient.on('message', (topic, message) => {
const timestamp = new Date().toLocaleTimeString();
console.log(`\n📨 [${timestamp}] Mesaj MQTT primit:`);
console.log(` Topic: ${topic}`);
console.log(` Date: ${message.toString()}`);
// Procesare specială pentru topicul de senzori
if (topic === 'automatizare/esp32/senzori') {
console.log(' ✨ Date senzori detectate!');
try {
const sensorData = JSON.parse(message.toString());
console.log(' Parsed data:', sensorData);
// Trimite datele către clienții WebSocket
io.emit('sensorData', {
topic: topic,
data: sensorData,
timestamp: new Date().toISOString()
});
} catch (err) {
console.log(' ⚠️ Date non-JSON:', message.toString());
// Trimite oricum datele
io.emit('sensorData', {
topic: topic,
rawData: message.toString(),
timestamp: new Date().toISOString()
});
}
return;
}
try {
const data = JSON.parse(message.toString());
// Procesează mesajele de la sistemul de relee
if (topic === 'esp/relay/status') {
console.log('Status releu primit:', data);
225
updateRelayState(data);
// Transmite statusul releelor către clienții WebSocket
io.emit('relayStatus', data);
return;
}
if (topic === 'esp/relay/heartbeat') {
console.log('Heartbeat ESP Bridge primit:', data);
io.emit('relayHeartbeat', data);
return;
}
if (topic === 'esp/mega/status') {
console.log('Status Arduino Mega primit:', data);
io.emit('megaStatus', data);
return;
}
// Salvează datele în baza de date dacă sunt date de senzori
if (topic.includes('/data')) {
const moduleId = topic.split('/')[1];
// Pentru fiecare tip de senzor, salvăm datele în baza de date
Object.entries(data).forEach(async ([sensorType, value]) => {
// Ignoră proprietățile care nu sunt date de senzori
if (['temperature', 'humidity', 'soilMoisture', 'light', 'co2', 'waterFlow'].includes(sensorType)) {
// Definim unitatea de măsură pentru fiecare tip de senzor
const units = {
temperature: '°C',
humidity: '%',
soilMoisture: '%',
light: 'lux',
co2: 'ppm',
waterFlow: 'L/min'
};
// Facem un request către ruta API pentru a salva datele
// Aceasta va folosi logica din controller și va genera alerte dacă e cazul
const requestData = {
moduleId,
sensorType,
226
value,
unit: units[sensorType] || ''
};
// Trimitem datele către API-ul nostru intern
fetch('http://localhost:' + (process.env.PORT || 5000) + '/api/data', {
method: 'POST',
headers: {
'Content-Type': 'application/json',
'Authorization': 'Bearer ' + process.env.INTERNAL_API_TOKEN
},
body: JSON.stringify(requestData)
}).catch(err => console.error('Eroare la salvarea datelor:', err));
}
});
}
// Transmite datele către clienții WebSocket
io.emit('sensorData', {
topic,
message: data
});
} catch (err) {
console.error('Eroare la procesarea mesajului MQTT:', err);
}
});
// Configurare WebSockets
io.on('connection', (socket) => {
console.log('New client connected');
// Trimite date inițiale către client
socket.emit('initialData', { message: 'Conectat la server' });
socket.on('command', (data) => {
console.log('Comandă primită:', data);
// Transmite comanda către MQTT
mqttClient.publish(`esp/${data.module}/command`, JSON.stringify(data));
});
socket.on('disconnect', () => {
console.log('Client disconnected');
227
});
});
// Import și folosire rute
const authRoutes = require('./routes/auth');
const dataRoutes = require('./routes/data');
const espRoutes = require('./routes/esp');
const alertsRoutes = require('./routes/alerts');
const jsonRoutes = require('./routes/json');
const relayRoutes = require('./routes/relays');
// Montare rute
app.use('/api/auth', authRoutes);
app.use('/api/data', dataRoutes);
app.use('/api/esp', espRoutes);
app.use('/api/alerts', alertsRoutes);
app.use('/api/json', jsonRoutes);
app.use('/api/relays', relayRoutes);
// Rută de test
app.get('/api/test', (req, res) => {
res.json({ message: 'API funcționează corect!' });
});
// Middleware pentru gestionarea erorilor
app.use(errorHandler);
const PORT = process.env.PORT || 5000;
server.listen(PORT, () => {
console.log(`Server running on port ${PORT}`);
});
// Oprește sistemul JSON la închiderea aplicației
process.on('SIGINT', () => {
console.log('Închidere aplicație...');
stopJsonSystem();
process.exit(0);
});
process.on('SIGTERM', () => {
console.log('Închidere aplicație...');
stopJsonSystem();
228
process.exit(0);
});
// Functia de verificare conexiuni date in timp real
function sendSensorData() {
const data = generateSensorData();
const message = JSON.stringify(data);
client.publish(TOPIC, message, (err) => {
if (err) {
console.error('❌ Eroare la trimitere:', err);
} else {
const timestamp = new Date().toLocaleTimeString();
console.log(`📨 [${timestamp}] Date trimise pe topicul: ${TOPIC}`);
console.log(' Temperatură:', data.temperature, '°C');
console.log(' Umiditate:', data.humidity, '%');
console.log(' Umiditate sol medie:', data.moisture_mean, '%');
console.log(' ---');
}
});
}

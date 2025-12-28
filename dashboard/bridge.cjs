const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const net = require('net');
const cors = require('cors');

const app = express();
app.use(cors());
app.use(express.json());

const PORT = 9224;
const UDS_PATH = '/run/automatelinux/automatelinux-daemon.sock';

const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Function to send command to UDS and return response
function sendToDaemon(commandObj) {
    return new Promise((resolve, reject) => {
        const client = net.createConnection(UDS_PATH);
        let response = '';

        client.on('connect', () => {
            client.write(JSON.stringify(commandObj) + '\n');
        });

        client.on('data', (data) => {
            response += data.toString();
            if (response.endsWith('\n')) {
                client.destroy();
                resolve(response);
            }
        });

        client.on('error', (err) => {
            reject(err);
        });

        // Timeout
        setTimeout(() => {
            client.destroy();
            reject(new Error('Daemon connection timeout'));
        }, 2000);
    });
}

// REST API for macros and filters
app.get('/api/macros', async (req, res) => {
    try {
        const result = await sendToDaemon({ command: 'getMacros' });
        res.json(JSON.parse(result));
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.post('/api/macros', async (req, res) => {
    try {
        const result = await sendToDaemon({
            command: 'updateMacros',
            value: JSON.stringify(req.body)
        });
        res.send(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.post('/api/filters', async (req, res) => {
    try {
        const result = await sendToDaemon({
            command: 'setEventFilters',
            value: JSON.stringify(req.body)
        });
        res.send(result);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

// WebSocket for live logs
wss.on('connection', (ws) => {
    console.log('Dashboard client connected');
    const daemonLogClient = net.createConnection(UDS_PATH);

    daemonLogClient.on('connect', () => {
        daemonLogClient.write(JSON.stringify({ command: 'registerLogListener' }) + '\n');
    });

    daemonLogClient.on('data', (data) => {
        if (ws.readyState === WebSocket.OPEN) {
            ws.send(data.toString());
        }
    });

    ws.on('close', () => {
        daemonLogClient.destroy();
    });

    daemonLogClient.on('error', (err) => {
        console.error('Daemon log client error:', err);
        ws.close();
    });
});

server.listen(PORT, () => {
    console.log(`Bridge listening on http://localhost:${PORT}`);
});

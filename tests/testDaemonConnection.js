const net = require('net');

const SOCKET_PATH = '/run/automatelinux/automatelinux-daemon.sock';

// This mimics the data sent by the Gnome Shell extension
const windowInfo = {
    wmClass: 'test-script',
    windowTitle: 'Testing from Node.js',
    pid: process.pid,
    xid: 12345,
    role: 'test-role',
    frameRect: { x: 0, y: 0, width: 100, height: 100 },
    outerRect: { x: 0, y: 0, width: 100, height: 100 },
    monitor: 0
};

const commandPayload = {
    command: 'activeWindowChanged',
    ...windowInfo
};

console.log('Attempting to connect to socket at:', SOCKET_PATH);

const client = net.createConnection({ path: SOCKET_PATH });

client.on('connect', () => {
    console.log('Successfully connected to daemon.');
    const payload = JSON.stringify(commandPayload) + '\n';
    console.log('Sending payload string:');
    console.log(payload);
    client.write(payload);
});

client.on('data', (data) => {
    console.log('Received response from daemon:');
    console.log(data.toString());
    client.end(); // Close the connection after receiving a response
});

client.on('error', (err) => {
    console.error('Error connecting to daemon:', err.message);
});

client.on('end', () => {
    console.log('Disconnected from daemon.');
});

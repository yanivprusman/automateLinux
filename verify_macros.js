const net = require('net');
const UDS_PATH = '/run/automatelinux/automatelinux-daemon.sock';

const client = net.createConnection(UDS_PATH);
let response = '';

client.on('connect', () => {
    client.write(JSON.stringify({ command: 'getMacros' }) + '\n');
});

client.on('data', (data) => {
    response += data.toString();
    if (response.endsWith('\n')) {
        console.log(JSON.stringify(JSON.parse(response), null, 2));
        client.destroy();
    }
});

client.on('error', (err) => {
    console.error('Error:', err);
    process.exit(1);
});

setTimeout(() => {
    client.destroy();
    console.error('Timeout');
    process.exit(1);
}, 2000);

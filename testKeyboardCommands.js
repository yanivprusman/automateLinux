const net = require('net');
const SOCKET_PATH = '/run/automatelinux/automatelinux-daemon.sock';

async function sendCommand(commandObj) {
    return new Promise((resolve, reject) => {
        const client = net.createConnection({ path: SOCKET_PATH });
        let response = '';

        client.on('connect', () => {
            client.write(JSON.stringify(commandObj) + '\n');
        });

        client.on('data', (data) => {
            response += data.toString();
            client.end();
        });

        client.on('end', () => {
            resolve(response);
        });

        client.on('error', (err) => {
            reject(err);
        });
    });
}

async function runTests() {
    try {
        console.log('Testing getKeyboard...');
        const initial = await sendCommand({ command: 'getKeyboard' });
        console.log('Initial state:', initial);

        console.log('Testing setKeyboard enable=false...');
        const setFalse = await sendCommand({ command: 'setKeyboard', enable: 'false' });
        console.log('Set false response:', setFalse);

        const afterFalse = await sendCommand({ command: 'getKeyboard' });
        console.log('State after false:', afterFalse);

        console.log('Testing setKeyboard enable=true...');
        const setTrue = await sendCommand({ command: 'setKeyboard', enable: 'true' });
        console.log('Set true response:', setTrue);

        const final = await sendCommand({ command: 'getKeyboard' });
        console.log('Final state:', final);

    } catch (err) {
        console.error('Test failed:', err);
    }
}

runTests();


import Gio from 'gi://Gio';
import Shell from 'gi://Shell';
import GLib from 'gi://GLib';

const ActiveWindowTrackerInterface = 
`<node>
  <interface name="com.example.ActiveWindowTracker">
    <signal name="ActiveWindowChanged">
      <arg name="windowInfo" type="a{ss}"/>
    </signal>
  </interface>
</node>`;

export default class ActiveWindowTracker {
    #dbus;
    #windowTracker;
    #signalId;
    enable() {
        this.#windowTracker = Shell.WindowTracker.get_default();
        this.#dbus = Gio.DBusExportedObject.wrapJSObject(
            ActiveWindowTrackerInterface,
            this
        );
        this.#dbus.export(
            Gio.DBus.session,
            '/com/example/ActiveWindowTracker'
        );
        this.#signalId = global.display.connect('notify::focus-window', 
            () => this.#onActiveWindowChanged());
    }
    disable() {
        if (this.#signalId) {
            global.display.disconnect(this.#signalId);
            this.#signalId = null;
        }
        if (this.#dbus) {
            this.#dbus.unexport();
            this.#dbus = null;
        }
        this.#windowTracker = null;
    }
    #onActiveWindowChanged() {
        const window = global.display.focus_window;
        const wmClass = window.get_wm_class() || 'unknown';
        // const command = `/home/yaniv/coding/automateLinux/utilities/sendKeys/sendKeys "${wmClass}"`;
        this.#daemon('setKeyboard', { keyboardName: wmClass });
        try {
            const subprocess = new Gio.Subprocess({
                argv: ['/bin/bash', '-c', command],
                flags: Gio.SubprocessFlags.NONE,
            });
            subprocess.init(null);
        } catch (error) {
            logError(error, 'Failed to execute test command');
        }
        if (wmClass.toLowerCase().includes('chrome') || wmClass.toLowerCase().includes('chromium')) {
            this.#checkChromeTab();
            // this.#logToFile('Chrome detected, pinging daemon...');
            // this.#pingDaemon();
        }
    }

    // #pingDaemon() {
    //     try {
    //         const client = new Gio.SocketClient();
    //         const address = new Gio.UnixSocketAddress({ path: '/run/automatelinux/automatelinux-daemon.sock' });
    //         client.connect_async(address, null, (client, res) => {  // Change GLib.PRIORITY_DEFAULT to null
    //             try {
    //                 const connection = client.connect_finish(res);
    //                 const out = connection.get_output_stream();
    //                 const dout = new Gio.DataOutputStream({ base_stream: out });
    //                 // dout.put_string('ping\n', null);
    //                 dout.put_string(JSON.stringify({command: 'ping'}) + '\n', null);
    //                 dout.flush(null);
    //                 const input = connection.get_input_stream();
    //                 const din = new Gio.DataInputStream({ base_stream: input });
    //                 const [line] = din.read_line_utf8(null);
    //                 this.#logToFile(`Daemon replied: ${line}`);
    //             } catch (e) {
    //                 this.#logToFile(`Socket error: ${e}`);
    //             }
    //         });
    //     } catch (e) {
    //         this.#logToFile(`Failed: ${e.message}`);
    //         logError(e, 'Socket operation failed');
    //     }
    // }

    #daemon(command, params = {}) {
        try {
            const client = new Gio.SocketClient();
            const address = new Gio.UnixSocketAddress({ path: '/run/automatelinux/automatelinux-daemon.sock' });
            client.connect_async(address, null, (client, res) => {
                try {
                    const connection = client.connect_finish(res);
                    const out = connection.get_output_stream();
                    const dout = new Gio.DataOutputStream({ base_stream: out });
                    const commandObj = { command: command, ...params };
                    dout.put_string(JSON.stringify(commandObj) + '\n', null);
                    dout.flush(null);
                    const input = connection.get_input_stream();
                    const din = new Gio.DataInputStream({ base_stream: input });
                    const [line] = din.read_line_utf8(null);
                    // this.#logToFile(`Daemon (${command}) replied: ${line}`);
                } catch (e) {
                    // this.#logToFile(`Socket error: ${e}`);
                }
            });
        } catch (e) {
            // this.#logToFile(`Failed to connect to daemon: ${e.message}`);
            logError(e, 'Socket operation failed');
        }
    }

    #checkChromeTab() {
        try {
            console.log('Checking Chrome tab...');
            const cmd = `curl -s http://localhost:9222/json | jq -r '.[] | select(.type=="page") | .url' | head -1`;
            const subprocess = new Gio.Subprocess({
                argv: ['/bin/bash', '-c', cmd],
                flags: Gio.SubprocessFlags.STDOUT_PIPE,
            });
            subprocess.init(null);
            subprocess.wait_async(null, (proc, result) => {
                try {
                    proc.wait_finish(result);
                    const stdout = proc.get_stdout_pipe();
                    const reader = new Gio.DataInputStream({
                        base_stream: stdout,
                    });
                    const [line] = reader.read_line_utf8(null);
                    console.log('Chrome tab URL:', line);
                    if (line && line.trim().length > 0 && line.includes('chatgpt.com')) {
                        console.log('ChatGPT tab found:', line);
                        // this.#logToFile('ChatGPT tab found: ' + line);
                        this.#chatGpt();
                    } else if (line) {
                        console.log('Active tab is not ChatGPT:', line);
                    } else {
                        console.log('No active page tabs found');
                    }
                } catch (e) {
                    console.warn('Failed to read Chrome tab URL:', e);
                }
            });
        } catch (error) {
            console.warn('Chrome debugging not available:', error);
        }
    }    
    #logToFile(message) {
        try {
            const dataDir = '/home/yaniv/coding/automateLinux/data';
            const file = Gio.File.new_for_path(`${dataDir}/chrome.log`);
            const timestamp = new Date().toISOString();
            const logEntry = `[${timestamp}] ${message}\n`;
            const outputStream = file.append_to(Gio.FileCreateFlags.NONE, null);
            const dataStream = new Gio.DataOutputStream({ base_stream: outputStream });
            dataStream.put_string(logEntry, null);
            dataStream.flush(null);
            dataStream.close(null);
            outputStream.close(null);
        } catch (error) {
            console.warn('Failed to log to file:', error);
        }
    }
    #chatGpt() {
        try {
            // const command = `/home/yaniv/coding/automateLinux/utilities/sendKeys/sendKeys keyH keyI`;
            const command = `/home/yaniv/coding/automateLinux/utilities/sendKeys/sendKeys keyH keyI backspace backspace`;
            const subprocess = new Gio.Subprocess({
                argv: ['/bin/bash', '-c', command],
                flags: Gio.SubprocessFlags.NONE,
            });
            subprocess.init(null);
        } catch (error) {
            logError(error, 'Failed to send keys to ChatGPT');
        }
    }
}   

//  test conversation "Git ignore file update" in ChatGPT hi
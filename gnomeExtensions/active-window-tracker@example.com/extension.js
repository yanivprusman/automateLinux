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

// Define a logging function that can be used globally within the extension file.
function _log(message) {
    try {
        const dataDir = '/home/yaniv/coding/automateLinux/data';
        const file = Gio.File.new_for_path(`${dataDir}/chrome.log`);
        const timestamp = new Date().toISOString();
        const logEntry = `[${timestamp}] ${message}\n`;

        // Use synchronous file operations for simplicity in this specific debugging context.
        // Note: Asynchronous is better for performance in a production extension.
        const fileStream = file.append_to(Gio.FileCreateFlags.NONE, null);
        fileStream.write_all(logEntry, null);
        fileStream.close(null);
    } catch (e) {
        console.warn(`Failed to log to file: ${e.message}`);
    }
}

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
        _log('Extension enabled and using subprocess communication.');
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
        _log('Extension disabled.');
    }

    #onActiveWindowChanged() {
        const window = global.display.focus_window;
        if (!window) return;

        const wmClass = window.get_wm_class() || 'unknown';
        
        // Let's also log that the event was triggered
        _log(`Active window changed: ${wmClass}`);

        const windowInfo = {
            wmClass: wmClass,
            windowTitle: window.get_title() || '',
            pid: window.get_pid(),
            xid: window.get_xid(),
            role: window.get_role() || '',
            frameRect: JSON.stringify(window.get_frame_rect()),
            outerRect: JSON.stringify(window.get_outer_rect()),
            monitor: window.get_monitor()
        };
        
        this.#callDaemonAsSubprocess('ping', {});
    }

    #callDaemonAsSubprocess(command, params = {}) {
        try {
            const daemonPath = '/home/yaniv/coding/automateLinux/daemon/main';
            let argv = [daemonPath, 'send', command];

            for (const key in params) {
                argv.push(`--${key}`);
                argv.push(String(params[key]));
            }
            
            _log(`Executing: ${argv.join(' ')}`);

            const subprocess = new Gio.Subprocess({
                argv: argv,
                flags: Gio.SubprocessFlags.STDOUT_PIPE | Gio.SubprocessFlags.STDERR_PIPE,
            });
            subprocess.init(null);

            subprocess.communicate_utf8_async(null, null, (proc, res) => {
                try {
                    const [ok, stdout, stderr] = proc.communicate_utf8_finish(res);
                    if (!ok) {
                        _log(`Subprocess failed. Stderr: ${stderr.trim()}`);
                        return;
                    }
                    _log(`Subprocess success. Stdout: ${stdout.trim()}`);
                    if (stderr) {
                         _log(`Subprocess Stderr: ${stderr.trim()}`);
                    }
                } catch (e) {
                    _log(`Error communicating with subprocess: ${e.message}`);
                }
            });

        } catch (e) {
            _log(`Error creating subprocess: ${e.message}`);
        }
    }
}
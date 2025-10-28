
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
        const command = `/home/yaniv/coding/automateLinux/evsieve/toggle/sendKeys "${wmClass}"`;
        try {
            const subprocess = new Gio.Subprocess({
                argv: ['/bin/bash', '-c', command],
                flags: Gio.SubprocessFlags.NONE,
            });
            subprocess.init(null);
        } catch (error) {
            logError(error, 'Failed to execute test command');
        }
    }    
}
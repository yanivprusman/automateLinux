
import Gio from 'gi://Gio';
import Shell from 'gi://Shell';
import GLib from 'gi://GLib';

const ActiveWindowTrackerInterface = 
`<node>
  <interface name="com.example.ActiveWindowTracker">
    <method name="getActiveWindow">
      <arg name="windowInfo" type="a{ss}" direction="out"/>
    </method>
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

        // Connect to window focus changes
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

    #getActiveWindowInfo() {
        const window = global.display.focus_window;
        if (!window) {
            return { 'status': 'no-window' };
        }

        const app = this.#windowTracker.get_window_app(window);
        return {
            'window-title': window.get_title() || '',
            'wm-class': window.get_wm_class() || '',
            'wm-instance': window.get_wm_class_instance() || '',
            'window-id': window.get_id().toString(),
            'app-id': app ? app.get_id() || '' : '',
            'app-name': app ? app.get_name() || '' : '',
        };
    }

    #onActiveWindowChanged() {
        const windowInfo = this.#getActiveWindowInfo();
        this.#dbus.emit_signal('ActiveWindowChanged',
            new GLib.Variant('(a{ss})', [windowInfo]));
    }

    // D-Bus method
    getActiveWindow() {
        return this.#getActiveWindowInfo();
    }
}
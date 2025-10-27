
import Gio from 'gi://Gio';
import Shell from 'gi://Shell';
import GLib from 'gi://GLib';

const ActiveWindowTrackerInterface = 
`<node>
  <interface name="com.example.ActiveWindowTracker">
    <method name="getActiveWindow">
      <arg name="windowInfo" type="a{ss}" direction="out"/>
    </method>
    <method name="getWindowClassKey">
      <arg name="key" type="s" direction="out"/>
    </method>
    <method name="emulateKeypress">
      <arg name="keyString" type="s" direction="in"/>
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

    // D-Bus method to get a single character key based on window class
    getWindowClassKey() {
        const windowInfo = this.#getActiveWindowInfo();
        const wmClass = windowInfo['wm-class'];
        
        switch (wmClass) {
            case 'gnome-terminal-server':
                return 't';
            case 'Code':
                return 'c';
            case 'google-chrome':
                return 'g';
            default:
                return '';
        }
    }

    // D-Bus method to emulate keypress events
    emulateKeypress(keyString) {
        // This function is a placeholder for key emulation
        // The actual key emulation should be handled by evsieve
        // This just logs the request for debugging
        log(`Key emulation requested: ${keyString}`);
        return;
    }
}
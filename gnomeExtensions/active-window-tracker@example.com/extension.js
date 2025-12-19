import Gio from 'gi://Gio';
import Shell from 'gi://Shell';
import GLib from 'gi://GLib';
import { Logger } from 'file:///home/yaniv/coding/automateLinux/gnomeExtensions/lib/logging.js';
import { DaemonConnector } from 'file:///home/yaniv/coding/automateLinux/gnomeExtensions/lib/daemon.js';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';

const LOG_FILE_PATH = GLib.build_filenamev([GLib.get_home_dir(), 'coding', 'automateLinux', 'data', 'gnome.log']);
const daemon_unix_domain_socket_path = '/run/automatelinux/automatelinux-daemon.sock';
export default class ActiveWindowTracker extends Extension {
    #windowTracker;
    #signalId;
    #logger;
    #daemon;
    shouldLog = false;
    enable() {
        this.#logger = new Logger(LOG_FILE_PATH, this.shouldLog);
        this.#daemon = new DaemonConnector(daemon_unix_domain_socket_path, this.#logger);
        this.#windowTracker = Shell.WindowTracker.get_default();
        this.#signalId = global.display.connect('notify::focus-window',
            () => this.#onActiveWindowChanged());
        this.#logger.log('Extension enabled and using socket communication.');
    }

    disable() {
        if (this.#signalId) {
            global.display.disconnect(this.#signalId);
            this.#signalId = null;
        }
        this.#windowTracker = null;
        this.#logger.log('Extension disabled.');
    }

    #onActiveWindowChanged() {
        const window = global.display.focus_window;
        if (!window || typeof window.get_wm_class !== 'function' || typeof window.get_title !== 'function' || typeof window.get_pid !== 'function' || typeof window.get_xid !== 'function' || typeof window.get_role !== 'function' || typeof window.get_frame_rect !== 'function' || typeof window.get_outer_rect !== 'function' || typeof window.get_monitor !== 'function') {
            this.#logger.log('Active window is not a valid window object or missing properties, skipping update.');
            return;
        }

        const wmClass = window.get_wm_class() || 'unknown';
        
        this.#logger.log(`Active window changed: ${wmClass}`);

        const windowInfo = {
            command: 'activeWindowChanged',
            wmClass: wmClass,
            windowTitle: window.get_title() || '',
            pid: String(window.get_pid()),
            xid: String(window.get_xid()),
            role: window.get_role() || '',
            frameRect: JSON.stringify(window.get_frame_rect()),
            outerRect: JSON.stringify(window.get_outer_rect()),
            monitor: String(window.get_monitor())
        };
        
        this.#daemon.connectAndSendMessage(windowInfo);
    }
}
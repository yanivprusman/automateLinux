import Gio from 'gi://Gio';
import Shell from 'gi://Shell';
import GLib from 'gi://GLib';
import { Logger } from '../lib/logging.js';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';
import { DaemonConnector } from '../lib/daemon.js';

const LOG_FILE_PATH = GLib.build_filenamev([GLib.get_home_dir(), 'coding', 'automateLinux', 'data', 'gnome.log']);
const daemon_unix_domain_socket_path = '/run/automatelinux/automatelinux-daemon.sock';
export default class ActiveWindowTracker extends Extension {
    #windowTracker;
    #signalId;
    #logger;
    #daemon;

    enable() {
        this.#logger = new Logger(LOG_FILE_PATH, true);
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
        if (!window) return;

        const wmClass = window.get_wm_class() || 'unknown';
        
        this.#logger.log(`Active window changed: ${wmClass}`);

        const windowInfo = {
            command: 'activeWindowChanged',
            wmClass: wmClass,
            windowTitle: window.get_title() || '',
            pid: String(window.get_pid()),
            xid: String(window.get_xid()),
        };
        
        this.#daemon.connectAndSendMessage(windowInfo);
    }
}
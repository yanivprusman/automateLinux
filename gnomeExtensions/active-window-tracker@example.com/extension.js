import GLib from 'gi://GLib';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';
import { Logger } from '../lib/logging.js';
import { DaemonConnector } from '../lib/daemon.js';
import { ShellCommandExecutor } from '../lib/shellCommand.js';
const DAEMON_SOCKET_PATH = '/run/automatelinux/automatelinux-daemon.sock';
const LOG_FILE_PATH = GLib.build_filenamev([GLib.get_home_dir(), 'coding', 'automateLinux', 'data', 'gnome.log']);
const shouldLog = true;
export default class ActiveWindowTracker extends Extension {
    constructor(metadata) {
        super(metadata);
        this.logger = new Logger(LOG_FILE_PATH, shouldLog);
        this.daemon = new DaemonConnector(DAEMON_SOCKET_PATH, this.logger);
        this.shellExecutor = new ShellCommandExecutor(this.logger);
        this.logger.log('ActiveWindowTrackerExtension constructor called');
    }
    #signalId;
    enable() {
        this.#signalId = global.display.connect('notify::focus-window', () => this.#onActiveWindowChanged());
        this.logger.log('ActiveWindowTrackerExtension enabled');
    }
    disable() {
        if (this.#signalId) {
            global.display.disconnect(this.#signalId);
            this.#signalId = null;
        }
        super.disable();
    }
    #getActiveWindowInfo() {
        const window = global.display.focus_window;
        if (!window) {
            return { 'status': 'no-window' };
        }
        let app = null;
        if (typeof window.get_application === 'function') {
            app = window.get_application();
        }
        return {
            'window-title': window.get_title() || '',
            'wm-class': window.get_wm_class() || '',
            'wm-instance': window.get_wm_class_instance() || '',
            'window-id': window.get_id().toString(),
        };
    }

    #onActiveWindowChanged() {
        const windowInfo = this.#getActiveWindowInfo();
        this.logger.log(`Active window changed: ${JSON.stringify(windowInfo)}`);
        this.daemon.sendMessage({ 'event': 'active-window-changed', 'data': windowInfo });
    }
}
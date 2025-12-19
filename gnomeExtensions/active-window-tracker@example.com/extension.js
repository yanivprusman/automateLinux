import { Logger } from 'file:///home/yaniv/coding/automateLinux/gnomeExtensions/lib/logging.js';
import { DaemonConnector } from 'file:///home/yaniv/coding/automateLinux/gnomeExtensions/lib/daemon.js';
import { ShellCommandExecutor } from 'file:///home/yaniv/coding/automateLinux/gnomeExtensions/lib/shellCommand.js';
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
    }
    #getActiveWindowInfo() {
        const window = global.display.focus_window;
        if (!window) {
            return { 'status': 'no-window' };
        }
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
        this.logger.log(`Active window changed: ${JSON.stringify(windowInfo)}`);
    }
}
'use strict';

import GObject from 'gi://GObject';
import St from 'gi://St';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import * as PanelMenu from 'resource:///org/gnome/shell/ui/panelMenu.js';
import * as PopupMenu from 'resource:///org/gnome/shell/ui/popupMenu.js';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';
import Meta from 'gi://Meta';
import Shell from 'gi://Shell';
import GLib from 'gi://GLib';
import Gio from 'gi://Gio';

import { Logger } from './lib/logging.js';

const LOG_FILE_PATH = GLib.build_filenamev([GLib.get_home_dir(), 'coding', 'automateLinux', 'data', 'gnome_window_selector.log']);
const DAEMON_SOCKET_PATH = '/run/automatelinux/automatelinux-daemon.sock';

const WindowMenuItem = GObject.registerClass(
    class WindowMenuItem extends PopupMenu.PopupMenuItem {
        _init(window, logger) {
            super._init(window.get_title());
            this._window = window;
            this._logger = logger;

            this.connect('activate', () => {
                this._logger.log(`Activating window from menu: ${this._window.get_title()}`);
                this._window.activate(global.get_current_time());
            });
        }
    });

const WindowSelectorButton = GObject.registerClass(
    class WindowSelectorButton extends PanelMenu.Button {
        _init(logger) {
            super._init(0.0, 'Window Selector', false);
            this._logger = logger;
            this._logger.log("WindowSelectorButton initialized");

            // Icon
            const icon = new St.Icon({
                icon_name: 'view-restore-symbolic',
                style_class: 'system-status-icon',
            });
            this.add_child(icon);

            this.menu.connect('open-state-changed', (menu, open) => {
                this._logger.log(`Menu state changed: open=${open}`);
                if (open) {
                    this._populateMenu();
                }
            });
        }

        _populateMenu() {
            this.menu.removeAll();
            this._logger.log("Populating menu...");

            try {
                // Get all windows
                const windows = global.display.get_tab_list(Meta.TabList.NORMAL, null);
                this._logger.log(`Found ${windows.length} windows`);

                if (windows.length === 0) {
                    this.menu.addMenuItem(new PopupMenu.PopupMenuItem("No windows", { reactive: false }));
                    return;
                }

                for (let window of windows) {
                    let item = new WindowMenuItem(window, this._logger);
                    this.menu.addMenuItem(item);
                }
            } catch (e) {
                this._logger.log(`Error populating menu: ${e.message}`);
                this.menu.addMenuItem(new PopupMenu.PopupMenuItem("Error listing windows", { reactive: false }));
            }
        }
    });

class PersistentDaemonConnector {
    constructor(socketPath, logger) {
        this.socketPath = socketPath;
        this.logger = logger;
        this.running = false;
        this.outputStream = null;
        this.inputStream = null;
        this.connection = null;
    }

    start() {
        this.running = true;
        this._connectLoop();
    }

    stop() {
        this.running = false;
        if (this.connection) {
            this.connection.close(null);
            this.connection = null;
        }
    }

    async _connectLoop() {
        while (this.running) {
            try {
                this.logger.log(`Connecting to daemon at ${this.socketPath}...`);
                const client = new Gio.SocketClient();
                const conn = await new Promise((resolve, reject) => {
                    client.connect_async(
                        new Gio.UnixSocketAddress({ path: this.socketPath }),
                        null,
                        (source, res) => {
                            try { resolve(client.connect_finish(res)); }
                            catch (e) { reject(e); }
                        }
                    );
                });

                this.connection = conn;
                this.outputStream = new Gio.DataOutputStream({ base_stream: conn.get_output_stream() });
                this.inputStream = new Gio.DataInputStream({ base_stream: conn.get_input_stream() });

                // Register
                const regCmd = JSON.stringify({ command: 'registerWindowExtension' }) + '\n';
                this.outputStream.put_string(regCmd, null);

                this.logger.log("Registered with daemon. Listening for commands...");

                // Read Loop
                while (this.running) {
                    const line = await new Promise((resolve, reject) => {
                        this.inputStream.read_line_async(GLib.PRIORITY_DEFAULT, null, (source, res) => {
                            try {
                                const [line] = source.read_line_finish_utf8(res);
                                resolve(line);
                            } catch (e) { reject(e); }
                        });
                    });

                    if (line === null) {
                        throw new Error("Connection closed by daemon");
                    }

                    if (line.trim()) {
                        await this._handleMessage(line);
                    }
                }

            } catch (e) {
                this.logger.log(`Daemon connection error: ${e.message}. Retrying in 2s...`);
                if (this.connection) {
                    try { this.connection.close(null); } catch (_) { }
                    this.connection = null;
                }
                await new Promise(r => setTimeout(r, 2000));
            }
        }
    }

    async _handleMessage(line) {
        this.logger.log(`Received from daemon: ${line}`);
        try {
            const msg = JSON.parse(line);
            if (msg.action === 'listWindows') {
                const windows = global.display.get_tab_list(Meta.TabList.NORMAL, null);
                const winList = windows.map(w => ({
                    id: w.get_id(),
                    title: w.get_title(),
                    wm_class: w.get_wm_class(),
                    workspace: w.get_workspace().index()
                }));
                const response = JSON.stringify(winList) + '\n';
                this.outputStream.put_string(response, null);
            } else if (msg.action === 'activateWindow') {
                const winId = parseInt(msg.windowId); // Assuming passed in msg
                // Need to find window by ID
                const windows = global.display.get_tab_list(Meta.TabList.NORMAL, null);
                const target = windows.find(w => w.get_id() === winId);

                let result = "not_found";
                if (target) {
                    // Activate needs to run on main thread? logic seems to be Main loop based anyway in extensions? 
                    // Usually yes.
                    target.activate(global.get_current_time());
                    target.raise();
                    if (target.get_workspace()) target.get_workspace().activate(global.get_current_time());
                    result = "activated";
                }

                const response = JSON.stringify({ status: result, id: winId }) + '\n';
                this.outputStream.put_string(response, null);
            }
        } catch (e) {
            this.logger.log(`Error handling message: ${e.message}`);
        }
    }
}

export default class WindowSelectorExtension extends Extension {
    enable() {
        this.logger = new Logger(LOG_FILE_PATH, true);
        this.logger.log("Extension enabled");

        this._indicator = new WindowSelectorButton(this.logger);
        Main.panel.addToStatusArea(this.uuid, this._indicator);

        this._daemonListener = new PersistentDaemonConnector(DAEMON_SOCKET_PATH, this.logger);
        this._daemonListener.start();
    }

    disable() {
        if (this.logger) this.logger.log("Extension disabled");
        if (this._indicator) {
            this._indicator.destroy();
            this._indicator = null;
        }
        if (this._daemonListener) {
            this._daemonListener.stop();
            this._daemonListener = null;
        }
    }
}

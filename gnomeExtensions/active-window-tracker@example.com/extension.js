'use strict';

import Gio from 'gi://Gio';
import GLib from 'gi://GLib';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';
import { Logger } from '../lib/logging.js';
import { DaemonConnector } from '../lib/daemon.js';

const DAEMON_SOCKET_PATH = '/run/automatelinux/automatelinux-daemon.sock';
const LOG_FILE_PATH = '/opt/automateLinux/data/gnome.log';

export default class ActiveWindowTracker extends Extension {
    constructor(metadata) {
        super(metadata);
        this.logger = new Logger(LOG_FILE_PATH, true);
        this.daemon = new DaemonConnector(DAEMON_SOCKET_PATH, this.logger);
        this._focusSignalId = null;
        this._dbusSignalId = 0;
    }

    enable() {
        this.logger.log('ActiveWindowTracker.enable() called');

        if (global.display) {
            this.logger.log('Setting up focus-window tracking...');
            this._focusSignalId = global.display.connect('notify::focus-window', () => this._onActiveWindowChanged());

            try {
                this.logger.log('Subscribing to daemon System DBus signals...');
                this._dbusSignalId = Gio.DBus.system.signal_subscribe(
                    null, // sender
                    'com.automatelinux.daemon', // interface
                    'Ready', // member
                    '/com/automatelinux/daemon', // object path
                    null, // arg0
                    Gio.DBusSignalFlags.NONE,
                    () => {
                        this.logger.log('Daemon ready signal received via System DBus!');
                        this._onActiveWindowChanged();
                    }
                );
            } catch (e) {
                this.logger.log(`Failed to subscribe to DBus: ${e.message}`);
            }

            // Initial sync
            this._onActiveWindowChanged();
        }
    }

    disable() {
        this.logger.log('ActiveWindowTracker.disable() called');
        if (this._focusSignalId) {
            global.display.disconnect(this._focusSignalId);
            this._focusSignalId = null;
        }
        if (this._dbusSignalId) {
            Gio.DBus.system.signal_unsubscribe(this._dbusSignalId);
            this._dbusSignalId = 0;
        }
    }

    _getActiveWindowInfo() {
        const window = global.display.get_focus_window();
        if (!window) return null;

        return {
            windowId: window.get_id(),
            windowTitle: window.get_title(),
            wmClass: window.get_wm_class(),
            wmInstance: window.get_wm_class_instance()
        };
    }

    async _onActiveWindowChanged() {
        try {
            const info = this._getActiveWindowInfo();
            if (!info) return;

            this.logger.log(`Active window changed: ${info.wmClass} - ${info.windowTitle}`);

            const message = {
                command: 'activeWindowChanged',
                ...info
            };

            await this.daemon.connectAndSendMessage(message);
        } catch (e) {
            this.logger.log(`Error in _onActiveWindowChanged: ${e.message}`);
        }
    }
}

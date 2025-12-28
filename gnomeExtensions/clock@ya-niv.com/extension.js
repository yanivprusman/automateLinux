'use strict';

import St from 'gi://St';
import GLib from 'gi://GLib';
import Clutter from 'gi://Clutter';
import GObject from 'gi://GObject';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import * as PopupMenu from 'resource:///org/gnome/shell/ui/popupMenu.js';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';

import { Logger } from '../lib/logging.js';
import { DaemonConnector } from '../lib/daemon.js';
import { ShellCommandExecutor } from '../lib/shellCommand.js';

const DAEMON_SOCKET_PATH = '/run/automatelinux/automatelinux-daemon.sock';
const LOG_FILE_PATH = GLib.build_filenamev([GLib.get_home_dir(), 'coding', 'automateLinux', 'data', 'gnome.log']);
const shouldLog = true;

// Custom menu item that closes on left-click, stays open on right-click
const NonClosingPopupSwitchMenuItem = GObject.registerClass(
    class NonClosingPopupSwitchMenuItem extends PopupMenu.PopupSwitchMenuItem {
        activate(event) {
            this.toggle();
        }
    });

export default class ClockExtension extends Extension {
    constructor(metadata) {
        super(metadata);
        this.logger = new Logger(LOG_FILE_PATH, shouldLog); //remove
        this.daemon = new DaemonConnector(DAEMON_SOCKET_PATH, this.logger);
        this.shellExecutor = new ShellCommandExecutor(this.logger);
        this._label = null;
        this._timeoutId = 0;
        this._lastX = null;
        this._lastY = null;
        this._menu = null;
        this._daemonLoggingEnabled = false;
        this._toggleLoggingMenuItem = null;
        this._toggleDaemonMenuItem = null;
        this._isDaemonActive = false;
        this._keyboardEnabled = true;
        this._toggleKeyboardMenuItem = null;
        this.logger.log('ClockExtension constructor called');
        this._isUserService = false;

    }

    async enable() {
        this.logger.log('ClockExtension.enable() called');
        this.logger.log(`Extension path: ${this.path}`);
        try {
            this._label = new St.Label({
                text: '00:00',
                style_class: 'clock-label',
                reactive: true,
                track_hover: true,
                y_align: Clutter.ActorAlign.START,
                x_align: Clutter.ActorAlign.START
            });
            this.logger.log('Label created');
            let x, y;
            const loadedPos = await this._loadPosition();
            if (loadedPos.x !== null && loadedPos.y !== null) {
                x = loadedPos.x;
                y = loadedPos.y;
                this.logger.log(`Loaded position used: X=${x}, Y=${y}`);
            } else {
                const monitorIndex = 0; // Primary monitor
                const monitor = Main.layoutManager.monitors[monitorIndex];
                if (!monitor) {
                    this.logger.log(`Monitor ${monitorIndex} not found, falling back to (50,50)`);
                    x = 50;
                    y = 50;
                } else {
                    x = monitor.x + 30;
                    y = monitor.y + 30;
                    this.logger.log(`Default position used: X=${x}, Y=${y}`);
                }
                this._savePosition(x, y);
            }
            this._label.set_position(x, y);
            this._lastX = x;
            this._lastY = y;
            this.logger.log(`Label positioned at ${x},${y}`);
            Main.uiGroup.add_child(this._label);
            this.logger.log('Label added to stage');
            this._label.show();
            this.logger.log('Label shown');
            this._menu = new PopupMenu.PopupMenu(this._label, 0.5, St.Side.TOP);
            Main.uiGroup.add_child(this._menu.actor);
            this._menu.actor.hide();

            try {
                let shutDownMenuItem = new PopupMenu.PopupMenuItem('Power Off');
                shutDownMenuItem.connect('activate', (menuItem, event) => {
                    if (event && event.get_button() !== 1) {
                        return;
                    }
                    this._onShutdownMenuItemActivated();
                });
                this._menu.addMenuItem(shutDownMenuItem);

                // Add separator
                this._menu.addMenuItem(new PopupMenu.PopupSeparatorMenuItem());

                // Add logging submenu
                this._loggingSubMenu = new PopupMenu.PopupSubMenuMenuItem('Daemon Logging');
                this._menu.addMenuItem(this._loggingSubMenu);

                this._logItems = {};
                const categories = [
                    { name: 'Input', mask: 1 },
                    { name: 'Window', mask: 2 },
                    { name: 'Automation', mask: 4 },
                    { name: 'Core', mask: 8 },
                    { name: 'Macro Debug', mask: 16 },
                    { name: 'Network', mask: 32 },
                    { name: 'Chrome', mask: 64 },
                    { name: 'Terminal', mask: 128 }
                ];

                categories.forEach(cat => {
                    let item = new NonClosingPopupSwitchMenuItem(cat.name, false);
                    item.connect('toggled', (obj, state) => this._onLogCategoryToggled(cat.mask, state));
                    this._loggingSubMenu.menu.addMenuItem(item);
                    this._logItems[cat.mask] = item;
                });

                // Add toggle daemon menu item
                this._toggleDaemonMenuItem = new PopupMenu.PopupMenuItem('Checking Daemon Status...');
                this._toggleDaemonMenuItem.connect('activate', () => this._onToggleDaemonActivated());
                this._menu.addMenuItem(this._toggleDaemonMenuItem);

                // Add toggle keyboard menu item
                this._toggleKeyboardMenuItem = new PopupMenu.PopupMenuItem('Enable Keyboard');
                this._toggleKeyboardMenuItem.connect('activate', () => this._onToggleKeyboardActivated());
                this._menu.addMenuItem(this._toggleKeyboardMenuItem);

                this._label.menu = this._menu;
                this._menu.connect('open-state-changed', (menu, open) => {
                    if (open) {
                        this._updateDaemonStatus();
                    }
                });
            } catch (menuError) {
                this.logger.log(`Error building menu: ${menuError.message}`);
                this.logger.log(`Stack: ${menuError.stack}`);
            } // Continue execution even if menu fails
            this._updateClock();
            this._timeoutId = GLib.timeout_add_seconds(
                GLib.PRIORITY_DEFAULT,
                1,
                () => {
                    this._updateClock();
                    const x = this._label.get_x();
                    const y = this._label.get_y();
                    if (x !== this._lastX || y !== this._lastY) {
                        this._savePosition(x, y);
                        this._lastX = x;
                        this._lastY = y;
                    }
                    return GLib.SOURCE_CONTINUE;
                }
            );
            this.logger.log('Clock update timer started');
            this._setupDragging();

            // Load initial daemon state
            this._loadDaemonLoggingState();
            this._loadKeyboardState();
            this._updateDaemonStatus();
        } catch (e) {
            this.logger.log(`Error in enable(): ${e.message}`);
            this.logger.log(`Stack: ${e.stack}`);
        }
    }

    _setupDragging() {
        let dragData = { dragging: false, offsetX: 0, offsetY: 0 };
        this._label.connect('button-press-event', (_, event) => {
            if (event.get_button() === 1) {
                if (this._menu.isOpen) {
                    this.logger.log('Left-click on label while menu is open - closing menu');
                    this._menu.close();
                }
                this.logger.log('Button pressed on label (left-click)');
                dragData.dragging = true;
                const [stageX, stageY] = event.get_coords();
                const actorX = this._label.get_x();
                const actorY = this._label.get_y();
                dragData.offsetX = stageX - actorX;
                dragData.offsetY = stageY - actorY;
                this.logger.log(`Drag start: stage(${stageX},${stageY}), offset(${dragData.offsetX},${dragData.offsetY})`);
                return Clutter.EVENT_STOP;
            } else if (event.get_button() === 3) {
                this.logger.log('Button pressed on label (right-click)');
                this._menu.toggle();
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });
        this._label.connect('button-release-event', (_, event) => {
            if (event.get_button() === 1) {
                this.logger.log('Button released');
                dragData.dragging = false;
                const x = this._label.get_x();
                const y = this._label.get_y();
                this.logger.log(`Drag end: position(${x},${y})`);
                this._savePosition(x, y);
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });
        this._label.connect('motion-event', (_, event) => {
            if (dragData.dragging) {
                const [stageX, stageY] = event.get_coords();
                const newX = stageX - dragData.offsetX;
                const newY = stageY - dragData.offsetY;
                this._label.set_position(newX, newY);
                this.logger.log(`Dragging: new position(${newX},${newY})`);
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });
    }

    async _onShutdownMenuItemActivated() {
        this.logger.log('Shut Down menu item activated. Executing systemctl poweroff...');
        this.shellExecutor.execute('/usr/bin/systemctl poweroff');
    }

    async _onLogCategoryToggled(mask, state) {
        try {
            // Update local state mask
            if (state) {
                this._daemonLoggingMask |= mask;
            } else {
                this._daemonLoggingMask &= ~mask;
            }

            // Send full mask to daemon
            const response = await this.daemon.connectAndSendMessage({
                command: 'shouldLog',
                enable: this._daemonLoggingMask.toString()
            });

            this.logger.log(`Daemon logging mask updated: ${this._daemonLoggingMask}, response: ${response}`);
        } catch (e) {
            this.logger.log(`Error updating logging mask: ${e.message}`);
        }
    }

    async _loadDaemonLoggingState() {
        try {
            const response = await this.daemon.connectAndSendMessage({
                command: 'getShouldLog'
            });

            if (response) {
                // If response is numeric, parse it.
                // If it's legacy 'true'/'false', map to ALL/NONE
                let mask = 0;
                const trimmed = response.trim();
                if (trimmed === 'true') mask = 15; // 1|2|4|8
                else if (trimmed === 'false') mask = 0;
                else {
                    const parsed = parseInt(trimmed);
                    if (!isNaN(parsed)) mask = parsed;
                }

                this._daemonLoggingMask = mask;

                // Update UI switches
                for (const [m, item] of Object.entries(this._logItems)) {
                    item.setToggleState((mask & parseInt(m)) !== 0);
                }
            }

            this.logger.log(`Loaded daemon logging mask: ${this._daemonLoggingMask}`);
        } catch (e) {
            this.logger.log(`Error loading daemon logging state: ${e.message}`);
        }
    }

    async _onToggleKeyboardActivated() {
        try {
            // Toggle the state
            this._keyboardEnabled = !this._keyboardEnabled;

            // Send command to daemon
            const response = await this.daemon.connectAndSendMessage({
                command: 'setKeyboard',
                enable: this._keyboardEnabled ? 'true' : 'false'
            });

            // Update menu item text
            this._toggleKeyboardMenuItem.label.text = this._keyboardEnabled
                ? 'Disable Keyboard'
                : 'Enable Keyboard';

            this.logger.log(`Keyboard toggled: ${this._keyboardEnabled}, response: ${response}`);
        } catch (e) {
            this.logger.log(`Error toggling keyboard: ${e.message}`);
        }
    }

    async _loadKeyboardState() {
        try {
            const response = await this.daemon.connectAndSendMessage({
                command: 'getKeyboard'
            });

            this._keyboardEnabled = response && response.trim() === 'true';

            // Update menu item text
            if (this._toggleKeyboardMenuItem) {
                this._toggleKeyboardMenuItem.label.text = this._keyboardEnabled
                    ? 'Disable Keyboard'
                    : 'Enable Keyboard';
            }

            this.logger.log(`Loaded keyboard state: ${this._keyboardEnabled}`);
        } catch (e) {
            this.logger.log(`Error loading keyboard state: ${e.message}`);
        }
    }

    _updateDaemonStatus() {
        try {
            // First check if it's a user service
            let output = this.shellExecutor.executeSync('systemctl --user show -p LoadState daemon.service');
            let loadState = output ? output.trim() : '';

            if (loadState === 'LoadState=loaded') {
                this._isUserService = true;
                output = this.shellExecutor.executeSync('systemctl --user is-active daemon.service');
            } else {
                // Not a user service, default to system service
                this._isUserService = false;
                output = this.shellExecutor.executeSync('systemctl is-active daemon.service');
            }

            let status = output ? output.trim() : 'unknown';

            this._isDaemonActive = (status === 'active');
            this.logger.log(`Daemon status check: ${status}, isActive: ${this._isDaemonActive}, isUser: ${this._isUserService}`);

            if (this._toggleDaemonMenuItem) {
                this._toggleDaemonMenuItem.label.text = this._isDaemonActive
                    ? 'Stop Daemon'
                    : 'Start Daemon';
            }
        } catch (e) {
            this.logger.log(`Error checking daemon status: ${e.message}`);
            if (this._toggleDaemonMenuItem) {
                this._toggleDaemonMenuItem.label.text = 'Daemon Status Error';
            }
        }
    }

    async _onToggleDaemonActivated() {
        this.logger.log(`Toggle Daemon activated. Current state: ${this._isDaemonActive ? 'Active' : 'Inactive'}`);

        let command;
        if (this._isDaemonActive) {
            // Stop daemon
            command = this._isUserService ? 'systemctl --user stop daemon.service' : 'pkexec systemctl stop daemon.service';
        } else {
            // Start daemon
            command = this._isUserService ? 'systemctl --user start daemon.service' : 'pkexec systemctl start daemon.service';
        }

        this.logger.log(`Executing command: ${command}`);
        this.shellExecutor.execute(command);

        // Wait a bit for the command to execute then update status
        // systemctl start/stop might take a moment.
        GLib.timeout_add(GLib.PRIORITY_DEFAULT, 1000, () => {
            this._updateDaemonStatus();
            return GLib.SOURCE_REMOVE;
        });

        // Also update immediately (optimistic UI update?) 
        // Better to just wait for the timeout or maybe show "Working..."
        if (this._toggleDaemonMenuItem) {
            this._toggleDaemonMenuItem.label.text = this._isDaemonActive ? 'Stopping...' : 'Starting...';
        }
    }

    async _loadPosition() {
        this.logger.log('Loading position using DaemonConnector...');
        let x = null;
        let y = null;
        try {
            const xResponse = await this.daemon.connectAndSendMessage({
                command: 'getEntry',
                key: 'clockX'
            });
            if (xResponse) {
                const xVal = parseInt(xResponse);
                if (!isNaN(xVal)) {
                    x = xVal;
                }
            }
            const yResponse = await this.daemon.connectAndSendMessage({
                command: 'getEntry',
                key: 'clockY'
            });
            if (yResponse) {
                const yVal = parseInt(yResponse);
                if (!isNaN(yVal)) {
                    y = yVal;
                }
            }
            this.logger.log(`Loaded position from daemon: X=${x}, Y=${y}`);
        } catch (e) {
            this.logger.log(`Error loading position from daemon: ${e.message}`);
        }
        return { x, y };
    }

    _savePosition(x, y) {
        this.logger.log(`Saving position using DaemonConnector: X=${Math.round(x)}, Y=${Math.round(y)}`);
        this.daemon.connectAndSendMessage({
            command: 'upsertEntry',
            key: 'clockX',
            value: Math.round(x).toString()
        }).then(response => {
            this.logger.log(`Daemon response for saving clockX: ${response}`);
        }).catch(e => {
            this.logger.log(`Error saving clockX to daemon: ${e.message}`);
        });
        this.daemon.connectAndSendMessage({
            command: 'upsertEntry',
            key: 'clockY',
            value: Math.round(y).toString()
        }).then(response => {
            this.logger.log(`Daemon response for saving clockY: ${response}`);
        }).catch(e => {
            this.logger.log(`Error saving clockY to daemon: ${e.message}`);
        });
    }

    disable() {
        this.logger.log('ClockExtension.disable() called');
        if (this._timeoutId) {
            GLib.source_remove(this._timeoutId);
            this._timeoutId = 0;
        }
        if (this._label) {
            this._label.destroy();
            this._label = null;
        }
        if (this._menu) {
            this._menu.destroy();
            this._menu = null;
        }
    }

    _updateClock() {
        if (!this._label) {
            return;
        }
        const t = new Date();
        this._label.text = t.toLocaleTimeString([], {
            hour: '2-digit',
            minute: '2-digit',
            hour12: false
        });
    }
}

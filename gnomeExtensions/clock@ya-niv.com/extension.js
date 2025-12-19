'use strict';

import St from 'gi://St';
import GLib from 'gi://GLib';
import Clutter from 'gi://Clutter';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import * as PopupMenu from 'resource:///org/gnome/shell/ui/popupMenu.js';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';

import { Logger } from '../lib/logging.js';
import { DaemonConnector } from '../lib/daemon.js';
import { ShellCommandExecutor } from '../lib/shellCommand.js';

const DAEMON_SOCKET_PATH = '/run/automatelinux/automatelinux-daemon.sock';
const LOG_FILE_PATH = GLib.build_filenamev([GLib.get_home_dir(), 'coding', 'automateLinux', 'data', 'gnome.log']);
const shouldLog = false; 

export default class ClockExtension extends Extension {
    constructor(metadata) {
        super(metadata);
        this.logger = new Logger(LOG_FILE_PATH, shouldLog);
        this.daemon = new DaemonConnector(DAEMON_SOCKET_PATH, this.logger);
        this.shellExecutor = new ShellCommandExecutor(this.logger);
        this._label = null;
        this._timeoutId = 0;
        this._lastX = null;
        this._lastY = null;
        this._menu = null;
        this.logger.log('ClockExtension constructor called');
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
                // If using default, immediately save it to persist
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

            // Setup the right-click menu
            this._menu = new PopupMenu.PopupMenu(this._label, 0.5, St.Side.TOP);
            Main.uiGroup.add_child(this._menu.actor);
            this._menu.actor.hide(); // Initially hide the menu

            let shutDownMenuItem = new PopupMenu.PopupMenuItem('Shut Down');
            shutDownMenuItem.connect('activate', () => this._onShutdownMenuItemActivated());
            this._menu.addMenuItem(shutDownMenuItem);

            this._label.menu = this._menu; // Associate menu with the label

            this._updateClock();
            
            this._timeoutId = GLib.timeout_add_seconds(
                GLib.PRIORITY_DEFAULT,
                1,
                () => {
                    this._updateClock();
                    
                    // Periodically save position
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
            
            // Setup dragging
            this._setupDragging();
            
        } catch (e) {
            this.logger.log(`Error in enable(): ${e.message}`);
            this.logger.log(`Stack: ${e.stack}`);
        }
    }

    _setupDragging() {
        let dragData = { dragging: false, offsetX: 0, offsetY: 0 };

        this._label.connect('button-press-event', (_, event) => {
            if (event.get_button() === 1) { // Left-click for dragging
                this.logger.log('Button pressed on label (left-click)');
                dragData.dragging = true;
                const [stageX, stageY] = event.get_coords();
                const actorX = this._label.get_x();
                const actorY = this._label.get_y();
                dragData.offsetX = stageX - actorX;
                dragData.offsetY = stageY - actorY;
                this.logger.log(`Drag start: stage(${stageX},${stageY}), offset(${dragData.offsetX},${dragData.offsetY})`);
                return Clutter.EVENT_STOP;
            } else if (event.get_button() === 3) { // Right-click for menu
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
        // No need to wait for a response as it's a direct system command.
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
        return {x, y};
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

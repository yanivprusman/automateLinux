'use strict';

import St from 'gi://St';
import GLib from 'gi://GLib';
import Clutter from 'gi://Clutter';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import * as PopupMenu from 'resource:///org/gnome/shell/ui/popupMenu.js';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';

import { Logger } from '../lib/logging.js';
import { DaemonConnector } from '../lib/daemon.js';

const DAEMON_SOCKET_PATH = '/run/automatelinux/automatelinux-daemon.sock';
const LOG_FILE_PATH = GLib.build_filenamev([GLib.get_home_dir(), 'coding', 'automateLinux', 'data', 'gnome.log']);


export default class ClockExtension extends Extension {
    constructor(metadata) {
        super(metadata);
        this.logger = new Logger(LOG_FILE_PATH, true);
        this.daemon = new DaemonConnector(DAEMON_SOCKET_PATH, this.logger);
        this._label = null;
        this._timeoutId = 0;
        this._lastX = null;
        this._lastY = null;
        this._menu = null;
        this.logger.log('ClockExtension constructor called');
    }

    async enable() {
        this.logger.log('ClockExtension.enable() called');
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
            let x = 50;
            let y = 50;

            const monitorIndex = 0; // Primary monitor
            const monitor = Main.layoutManager.monitors[monitorIndex];
            
            if (!monitor) {
                this.logger.log(`Monitor ${monitorIndex} not found, falling back to (50,50)`);
            } else {
                x = monitor.x + 30;
                y = monitor.y + 30;
            }
            
            this._label.set_position(x, y);
            this._lastX = x;
            this._lastY = y;
            this.logger.log(`Label positioned at ${x},${y}`);
            
            Main.uiGroup.add_child(this._label);
            this.logger.log('Label added to stage');
            
            this._label.show();
            this.logger.log('Label shown');
            
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
            if (event.get_button() === 1) {
                this.logger.log('Button pressed on label');
                dragData.dragging = true;
                const [stageX, stageY] = event.get_coords();
                const actorX = this._label.get_x();
                const actorY = this._label.get_y();
                dragData.offsetX = stageX - actorX;
                dragData.offsetY = stageY - actorY;
                this.logger.log(`Drag start: stage(${stageX},${stageY}), offset(${dragData.offsetX},${dragData.offsetY})`);
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

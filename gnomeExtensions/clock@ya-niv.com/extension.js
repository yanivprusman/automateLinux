'use strict';

import St from 'gi://St';
import GLib from 'gi://GLib';
import Clutter from 'gi://Clutter';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';
import { Logger } from '../lib/logging.js';
import { DaemonConnector } from '../lib/daemon.js';

const LOG_FILE_PATH = GLib.build_filenamev([GLib.get_home_dir(), 'coding', 'automateLinux', 'data', 'gnome.log']);
const DAEMON_SOCKET_PATH = '/run/automatelinux/automatelinux-daemon.sock';

export default class ClockExtension extends Extension {
    #label = null;
    #timeoutId = 0;
    #logger;
    #daemon;

    async enable() {
        this.#logger = new Logger(LOG_FILE_PATH, true);
        this.#daemon = new DaemonConnector(DAEMON_SOCKET_PATH, this.#logger);
        this.#logger.log('ClockExtension enabling...');

        try {
            this.#label = new St.Label({
                text: '00:00',
                style_class: 'clock-label',
                reactive: true,
                track_hover: true
            });
            this.#logger.log('Label created');

            Main.uiGroup.add_child(this.#label);

            // Load position from daemon
            let x = 50, y = 50; // Default position
            const response = await this.#daemon.connectAndSendMessage({ command: 'getClockLocation' });
            if (response) {
                try {
                    const pos = JSON.parse(response);
                    if (pos.x !== undefined && pos.y !== undefined) {
                        x = pos.x;
                        y = pos.y;
                        this.#logger.log(`Loaded position from daemon: ${x}, ${y}`);
                    }
                } catch (e) {
                    this.#logger.log(`Failed to parse position from daemon: ${e}`);
                }
            } else {
                this.#logger.log('No position found on daemon, using default.');
            }
            
            this.#label.set_position(x, y);
            this.#logger.log(`Label positioned at ${x},${y}`);

            this._updateClock();
            
            this.#timeoutId = GLib.timeout_add_seconds(
                GLib.PRIORITY_DEFAULT,
                1,
                () => {
                    this._updateClock();
                    return GLib.SOURCE_CONTINUE; // Keep the timer running
                }
            );
            this.#logger.log('Clock update timer started');

            this._setupDragging();
            
        } catch (e) {
            this.#logger.log(`Error in enable(): ${e}. Stack: ${e.stack}`);
        }
    }

    _setupDragging() {
        let dragData = { dragging: false, offsetX: 0, offsetY: 0 };

        this.#label.connect('button-press-event', (_, event) => {
            if (event.get_button() === 1) {
                this.#logger.log('Drag start.');
                dragData.dragging = true;
                const [stageX, stageY] = event.get_coords();
                dragData.offsetX = stageX - this.#label.get_x();
                dragData.offsetY = stageY - this.#label.get_y();
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });

        this.#label.connect('button-release-event', (_, event) => {
            if (event.get_button() === 1 && dragData.dragging) {
                dragData.dragging = false;
                const x = Math.round(this.#label.get_x());
                const y = Math.round(this.#label.get_y());
                this.#logger.log(`Drag end. Saving position: ${x}, ${y}`);
                this.#daemon.connectAndSendMessage({ command: 'setClockLocation', x, y });
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });

        this.#label.connect('motion-event', (_, event) => {
            if (dragData.dragging) {
                const [stageX, stageY] = event.get_coords();
                this.#label.set_position(stageX - dragData.offsetX, stageY - dragData.offsetY);
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });
    }

    disable() {
        this.#logger.log('ClockExtension disabling...');
        if (this.#timeoutId) {
            GLib.source_remove(this.#timeoutId);
            this.#timeoutId = 0;
        }

        if (this.#label) {
            this.#label.destroy();
            this.#label = null;
        }
    }

    _updateClock() {
        if (!this.#label) {
            return;
        }
        
        const t = new Date();
        this.#label.text = t.toLocaleTimeString([], {
            hour: '2-digit',
            minute: '2-digit',
            hour12: false
        });
    }
}
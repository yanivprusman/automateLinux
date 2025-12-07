'use strict';

import St from 'gi://St';
import GLib from 'gi://GLib';
import Clutter from 'gi://Clutter';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';

export default class ClockExtension extends Extension {
    constructor(metadata) {
        super(metadata);
        this._label = null;
        this._timeoutId = 0;
        this._saveTimeoutId = 0;
        this._pendingX = null;
        this._pendingY = null;
        
        // Load socket path from daemon helper script
        try {
            const [success, stdout] = GLib.spawn_command_line_sync('bash /home/yaniv/coding/automateLinux/daemon/getSocketPath.sh');
            this._socketPath = success ? new TextDecoder().decode(stdout).trim() : '/run/automatelinux/automatelinux-daemon.sock';
        } catch (e) {
            console.warn('Failed to load socket path from script, using default:', e);
            this._socketPath = '/run/automatelinux/automatelinux-daemon.sock';
        }
    }

    _savePosition(x, y) {
        // Store pending position
        this._pendingX = Math.round(x);
        this._pendingY = Math.round(y);
        
        // Clear any existing timeout
        if (this._saveTimeoutId) {
            GLib.source_remove(this._saveTimeoutId);
        }
        
        // Debounce: save after 500ms of inactivity
        this._saveTimeoutId = GLib.timeout_add(GLib.PRIORITY_DEFAULT, 500, () => {
            this._doSavePosition();
            this._saveTimeoutId = 0;
            return GLib.SOURCE_REMOVE;
        });
    }

    _doSavePosition() {
        if (this._pendingX === null || this._pendingY === null) {
            return;
        }
        
        const xJson = JSON.stringify({command: 'upsertEntry', key: 'clockPositionX', value: String(this._pendingX)});
        const yJson = JSON.stringify({command: 'upsertEntry', key: 'clockPositionY', value: String(this._pendingY)});
        
        try {
            GLib.spawn_command_line_sync(`echo '${xJson}' | nc -U -q 1 ${this._socketPath}`);
            GLib.spawn_command_line_sync(`echo '${yJson}' | nc -U -q 1 ${this._socketPath}`);
            console.log(`Saved clock position: X=${this._pendingX}, Y=${this._pendingY}`);
        } catch (e) {
            console.warn('Failed to save clock position:', e);
        }
    }

    _loadPosition() {
        try {
            const procX = GLib.spawn_command_line_sync(`echo '{"command":"getEntry", "key":"clockPositionX"}' | nc -U -q 1 ${this._socketPath}`);
            const procY = GLib.spawn_command_line_sync(`echo '{"command":"getEntry", "key":"clockPositionY"}' | nc -U -q 1 ${this._socketPath}`);
            
            let x = null, y = null;
            if (procX[0]) {
                const valueX = new TextDecoder().decode(procX[1]).trim();
                x = valueX ? parseInt(valueX) : null;
            }
            if (procY[0]) {
                const valueY = new TextDecoder().decode(procY[1]).trim();
                y = valueY ? parseInt(valueY) : null;
            }
            return {x, y};
        } catch (e) {
            console.log('Failed to load position from daemon:', e);
        }
        return {x: null, y: null};
    }

    enable() {
        const monitorIndex = 1;
        const monitor = Main.layoutManager.monitors[monitorIndex];
        
        if (!monitor) {
            console.error(`Monitor ${monitorIndex} not found`);
            return;
        }

        this._label = new St.Label({
            style_class: 'clock-label',
            y_align: Clutter.ActorAlign.START,
            x_align: Clutter.ActorAlign.START,
            reactive: true,
            track_hover: true
        });

        Main.uiGroup.add_child(this._label);
        
        const savedPos = this._loadPosition();
        let posX = savedPos.x !== null ? savedPos.x : monitor.x + 30;
        let posY = savedPos.y !== null ? savedPos.y : monitor.y + 30;
        
        this._label.set_position(posX, posY);

        this._updateClock();
        
        this._timeoutId = GLib.timeout_add_seconds(
            GLib.PRIORITY_DEFAULT,
            1,
            () => {
                this._updateClock();
                return GLib.SOURCE_CONTINUE;
            }
        );

        // Dragging support
        let dragData = { dragging: false, offsetX: 0, offsetY: 0 };

        this._label.connect('button-press-event', (_, event) => {
            if (event.get_button() === 1) {
                dragData.dragging = true;
                const [x, y] = event.get_coords();
                dragData.offsetX = x - this._label.x;
                dragData.offsetY = y - this._label.y;
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });

        this._label.connect('button-release-event', (_, event) => {
            if (event.get_button() === 1) {
                dragData.dragging = false;
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });

        this._label.connect('motion-event', (_, event) => {
            if (dragData.dragging) {
                const [x, y] = event.get_coords();
                this._label.set_position(x - dragData.offsetX, y - dragData.offsetY);
                this._savePosition(x - dragData.offsetX, y - dragData.offsetY);
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });
    }

    disable() {
        if (this._timeoutId) {
            GLib.source_remove(this._timeoutId);
            this._timeoutId = 0;
        }

        if (this._saveTimeoutId) {
            GLib.source_remove(this._saveTimeoutId);
            this._saveTimeoutId = 0;
            // Flush any pending saves before disabling
            this._doSavePosition();
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
            // second: '2-digit',
            hour12: false
        });
    }
}
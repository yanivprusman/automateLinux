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
            reactive: true,       // make it respond to pointer events
            track_hover: true
        });

        Main.uiGroup.add_child(this._label);
        
        let posX = monitor.x + 30;
        let posY = monitor.y + 30;

        let storedPosX = this._runDaemonCommand("getEntry", { key: "clockPositionX" });
        let storedPosY = this._runDaemonCommand("getEntry", { key: "clockPositionY" });
        
        if (storedPosX !== null && storedPosY !== null && storedPosX !== "" && storedPosY !== "") {
            posX = parseInt(storedPosX.trim());
            posY = parseInt(storedPosY.trim());
            this._logError(`Loaded position: x=${posX}, y=${posY}`);
        } else {
            this._logError("No stored position found, using default.");
        }

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
                this._runDaemonCommand("upsertEntry", { key: "clockPositionX", value: this._label.x.toString() });
                this._runDaemonCommand("upsertEntry", { key: "clockPositionY", value: this._label.y.toString() });
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });

        this._label.connect('motion-event', (_, event) => {
            if (dragData.dragging) {
                const [x, y] = event.get_coords();
                this._label.set_position(x - dragData.offsetX, y - dragData.offsetY);
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });
    }

    disable() {
        if (this._label) {
            // Save the current position before destroying the label
            this._runDaemonCommand("upsertEntry", { key: "clockPositionX", value: this._label.x.toString() });
            this._runDaemonCommand("upsertEntry", { key: "clockPositionY", value: this._label.y.toString() });
        }

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
            // second: '2-digit',
            hour12: false
        });
    }

    _logError(message) {
        console.error(`[ClockExtension] ${message}`);
    }

    _runDaemonCommand(command, args = {}) {
        let commandLine = `daemon send ${command}`;
        for (const key in args) {
            commandLine += ` --${key} "${args[key]}"`;
        }

        try {
            let [res, stdout, stderr, exit_status] = GLib.spawn_command_line_sync(commandLine);
            
            if (exit_status !== 0) {
                this._logError(`Daemon command failed: ${commandLine}\nStderr: ${stderr.toString()}`);
                return null;
            }

            return stdout.toString().trim();
        } catch (e) {
            this._logError(`Exception running daemon command: ${e.message}\nCommand: ${commandLine}`);
            return null;
        }
    }
}
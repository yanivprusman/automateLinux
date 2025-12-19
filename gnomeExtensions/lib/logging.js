'use strict';

import Gio from 'gi://Gio';
import GLib from 'gi://GLib';

export class Logger {
    constructor(logFilePath, enableLogging = true) {
        this._logFilePath = logFilePath;
        this._enableLogging = enableLogging;
    }

    get isEnabled() {
        return this._enableLogging;
    }

    set isEnabled(value) {
        this._enableLogging = !!value;
    }

    log(text) {
        if (!this._enableLogging) {
            return;
        }

        try {
            const now = GLib.DateTime.new_now_local();
            const timeString = now.format('%Y-%m-%d %H:%M:%S');
            const file = Gio.File.new_for_path(this._logFilePath);
            const stream = file.append_to(Gio.FileCreateFlags.NONE, null);
            const bytes = new TextEncoder().encode(`[${timeString}] ${text}\n`);
            stream.write_all(bytes, null);
            stream.close(null);
        } catch (e) {
            console.error(`Failed to write to log file '${this._logFilePath}': ${e.message}`);
        }
    }
}

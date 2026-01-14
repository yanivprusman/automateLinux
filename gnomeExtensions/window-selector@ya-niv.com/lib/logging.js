'use strict';

import Gio from 'gi://Gio';
import GLib from 'gi://GLib';

export class Logger {
    constructor(logFilePath, enableLogging = true) {
        this._logFilePath = logFilePath;
        this._enableLogging = enableLogging;

        // Ensure the log directory exists
        try {
            const file = Gio.File.new_for_path(this._logFilePath);
            const parent = file.get_parent();
            if (parent && !parent.query_exists(null)) {
                parent.make_directory_with_parents(null);
            }
        } catch (e) {
            console.error(`Failed to create log directory for '${this._logFilePath}': ${e.message}`);
        }
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

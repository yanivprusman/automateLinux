// gnomeExtensions/lib/shellCommand.js
import GLib from 'gi://GLib';

import { Logger } from './logging.js';

export class ShellCommandExecutor {
    constructor(logger) {
        this.logger = logger || new Logger('/dev/null', false); // Fallback to a no-op logger if none provided
    }

    execute(command) {
        this.logger.log(`Executing shell command: ${command}`);
        try {
            GLib.spawn_command_line_async(command);
            this.logger.log(`Command '${command}' executed asynchronously.`);
        } catch (e) {
            this.logger.log(`Error executing command '${command}': ${e.message}`);
            this.logger.log(`Stack: ${e.stack}`);
        }
    }

    executeSync(command) {
        this.logger.log(`Executing sync shell command: ${command}`);
        try {
            let [success, stdout, stderr, exit_status] = GLib.spawn_command_line_sync(command);
            if (!success) {
                this.logger.log(`Command '${command}' failed to spawn.`);
                return null;
            }
            // Decode stdout/stderr if needed, but for simple status checks, exit_status/stdout might be enough.
            // Using ByteArray to decode if available, or just returning raw bytes/string construction if simplistic.
            // GLib.spawn_command_line_sync returns Uint8Arrays in GJS usually.
            const decoder = new TextDecoder('utf-8');
            const output = decoder.decode(stdout);

            this.logger.log(`Command '${command}' executed synchronously. Status: ${exit_status}`);
            return output;
        } catch (e) {
            this.logger.log(`Error executing sync command '${command}': ${e.message}`);
            return null;
        }
    }
}

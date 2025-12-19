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
}

'use strict';

import St from 'gi://St';
import GLib from 'gi://GLib';
import Clutter from 'gi://Clutter';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import Gio from 'gi://Gio';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';

const LOG_FILE_PATH = GLib.build_filenamev([GLib.get_home_dir(), 'coding', 'automateLinux', 'data', 'gnome.log']);
const daemon_unix_domain_socket_path = '/run/automatelinux/automatelinux-daemon.sock';
export default class ClockExtension extends Extension {
    enable() {
        this.appendToLog('Clock Extension Enabled');
        this.connectAndSendMessage({ command: 'ping' });
    }

    disable() {
        const timestamp = new Date().toISOString();
        this.appendToLog(`Clock Extension Disabled at ${timestamp}`);
    }

    async connectAndSendMessage(command) {
        this.appendToLog(`Attempting to send command to daemon: ${JSON.stringify(command)}`);
        const client = new Gio.SocketClient();
        let conn;
        try {
            const cancellable = new Gio.Cancellable();
            conn = await new Promise((resolve, reject) => {
                client.connect_async(
                    new Gio.UnixSocketAddress({ path: daemon_unix_domain_socket_path }),
                    cancellable,
                    (source, res) => {
                        try {
                            resolve(client.connect_finish(res));
                        } catch (e) {
                            reject(e);
                        }
                    }
                );
            });

            const outputStream = conn.get_output_stream();
            const dataOutputStream = new Gio.DataOutputStream({
                base_stream: outputStream,
            });

            const inputStream = conn.get_input_stream();
            const dataInputStream = new Gio.DataInputStream({
                base_stream: inputStream,
            });

            const jsonMessage = JSON.stringify(command);
            this.appendToLog(`Sending: ${jsonMessage}`);
            const bytes = new TextEncoder().encode(jsonMessage + '\n'); // Add newline for line-based protocols
            await new Promise((resolve, reject) => {
                dataOutputStream.write_bytes_async(
                    new GLib.Bytes(bytes),
                    GLib.PRIORITY_DEFAULT,
                    cancellable,
                    (source, res) => {
                        try {
                            dataOutputStream.write_bytes_finish(res);
                            resolve();
                        } catch (e) {
                            reject(e);
                        }
                    }
                );
            });
            await new Promise((resolve, reject) => {
                dataOutputStream.flush_async(
                    GLib.PRIORITY_DEFAULT,
                    cancellable,
                    (source, res) => {
                        try {
                            dataOutputStream.flush_finish(res);
                            resolve();
                        } catch (e) {
                            reject(e);
                        }
                    }
                );
            });


            this.appendToLog('Message sent, waiting for response...');

            const line = await this._read_line_async(dataInputStream, cancellable);

            if (line !== null) {
                this.appendToLog(`Daemon response: ${line.trim()}`);
            } else {
                this.appendToLog('No response from daemon or connection closed.');
            }

        } catch (e) {
            this.appendToLog(`Error communicating with daemon: ${e.message}`);
        } finally {
            if (conn) {
                conn.close(null);
            }
        }
    }
    
    // Helper to read a line asynchronously
    _read_line_async(dataInputStream, cancellable) {
        return new Promise((resolve, reject) => {
            dataInputStream.read_line_async(
                GLib.PRIORITY_DEFAULT,
                cancellable,
                (source, res) => {
                    try {
                        const [buffer, length] = source.read_line_finish_utf8(res);
                        resolve(buffer); // buffer contains the line, or null at EOF
                    } catch (e) {
                        reject(e);
                    }
                }
            );
        });
    }

    appendToLog(text) {
        const now = GLib.DateTime.new_now_local();
        const timeString = now.format('%Y-%m-%d %H:%M:%S');
        const file = Gio.File.new_for_path(LOG_FILE_PATH);
        const stream = file.append_to(Gio.FileCreateFlags.NONE, null);
        const bytes = new TextEncoder().encode(`[${timeString}] ${text}\n`);
        stream.write_all(bytes, null);
        stream.close(null);
    }
}

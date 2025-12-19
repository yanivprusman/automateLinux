'use strict';

import Gio from 'gi://Gio';
import GLib from 'gi://GLib';

export class DaemonConnector {
    #socketPath;
    #logger;
    constructor(socketPath, logger) {
        this.#socketPath = socketPath;
        this.#logger = logger;
    }

    async connectAndSendMessage(command) {
        const cancellable = new Gio.Cancellable();
        let conn;

        // Retry until the socket exists
        while (!this.#socketExists()) {
            this.#logger.log(`Socket not found at ${this.#socketPath}, retrying in 0.3s...`);
            await this.#sleep(300);
        }

        const client = new Gio.SocketClient();
        try {
            conn = await new Promise((resolve, reject) => {
                client.connect_async(
                    new Gio.UnixSocketAddress({ path: this.#socketPath }),
                    cancellable,
                    (source, res) => {
                        try { resolve(client.connect_finish(res)); }
                        catch (e) { reject(e); }
                    }
                );
            });

            const outputStream = conn.get_output_stream();
            const dataOutputStream = new Gio.DataOutputStream({ base_stream: outputStream });
            const inputStream = conn.get_input_stream();
            const dataInputStream = new Gio.DataInputStream({ base_stream: inputStream });

            const jsonMessage = JSON.stringify(command);
            this.#logger.log(`Sending: ${jsonMessage}`);
            const bytes = new TextEncoder().encode(jsonMessage + '\n');

            await new Promise((resolve, reject) => {
                dataOutputStream.write_bytes_async(
                    new GLib.Bytes(bytes),
                    GLib.PRIORITY_DEFAULT,
                    cancellable,
                    (source, res) => {
                        try { dataOutputStream.write_bytes_finish(res); resolve(); }
                        catch (e) { reject(e); }
                    }
                );
            });

            await new Promise((resolve, reject) => {
                dataOutputStream.flush_async(
                    GLib.PRIORITY_DEFAULT,
                    cancellable,
                    (source, res) => {
                        try { dataOutputStream.flush_finish(res); resolve(); }
                        catch (e) { reject(e); }
                    }
                );
            });

            this.#logger.log('Message sent, waiting for response...');
            const line = await this.#readLineAsync(dataInputStream, cancellable);

            if (line !== null) {
                const response = line.trim();
                this.#logger.log(`Daemon response: ${response}`);
                return response;
            } else {
                this.#logger.log('No response from daemon or connection closed.');
                return null;
            }

        } catch (e) {
            this.#logger.log(`Error communicating with daemon: ${e.message}`);
            return null;
        } finally {
            if (conn) conn.close(null);
        }
    }

    #readLineAsync(dataInputStream, cancellable) {
        return new Promise((resolve, reject) => {
            dataInputStream.read_line_async(
                GLib.PRIORITY_DEFAULT,
                cancellable,
                (source, res) => {
                    try { const [buffer] = source.read_line_finish_utf8(res); resolve(buffer); }
                    catch (e) { reject(e); }
                }
            );
        });
    }

    #socketExists() {
        try {
            const file = Gio.File.new_for_path(this.#socketPath);
            return file.query_exists(null);
        } catch {
            return false;
        }
    }

    #sleep(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
}

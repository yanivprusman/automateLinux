'use strict';

import St from 'gi://St';
import GLib from 'gi://GLib';
import Gio from 'gi://Gio';
import Clutter from 'gi://Clutter';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import * as PopupMenu from 'resource:///org/gnome/shell/ui/popupMenu.js';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';

const AUTOMATE_LINUX_SOCKET_PATH = "/run/automatelinux/automatelinux-daemon.sock";

export default class ClockExtension extends Extension {
    constructor(metadata) {
        super(metadata);
        this._label = null;
        this._timeoutId = 0;
        this._lastX = null;
        this._lastY = null;
        this._menu = null;
        this._socketClient = null; // Initialize socketClient
        console.log('ClockExtension constructor called');
    }

    enable() {
        console.log('ClockExtension.enable() called');
        try {
            this._label = new St.Label({
                text: '00:00',
                style_class: 'clock-label',
                reactive: true,
                track_hover: true
            });
            console.log('Label created');
            
            // Use global.window_group for draggable elements that should overlay everything
            global.window_group.add_child(this._label);
            console.log('Label added to global.window_group');
            
            // Load saved position or use default
            let {x, y} = this._loadPosition();
            if (x === null || y === null) {
                x = 50;
                y = 50;
            }
            this._label.set_position(x, y);
            this._lastX = x;
            this._lastY = y;
            console.log(`Label positioned at ${x},${y}`);
            
            this._label.show();
            console.log('Label shown');
            
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
            console.log('Clock update timer started');
            
            // Setup dragging
            this._setupDragging();
            
        } catch (e) {
            console.error('Error in enable():', e);
            console.error('Stack:', e.stack);
        }
    }

    _setupDragging() {
        let dragData = { dragging: false, offsetX: 0, offsetY: 0 };

        this._label.connect('button-press-event', (_, event) => {
            if (event.get_button() === 3) {
                // Right-click - show menu
                console.log('Right-click on label');
                this._showMenu();
                return Clutter.EVENT_STOP;
            } else if (event.get_button() === 1) {
                // Left-click - start dragging
                console.log('Button pressed on label');
                dragData.dragging = true;
                const [stageX, stageY] = event.get_coords();
                const actorX = this._label.get_x();
                const actorY = this._label.get_y();
                dragData.offsetX = stageX - actorX;
                dragData.offsetY = stageY - actorY;
                console.log(`Drag start: stage(${stageX},${stageY}), offset(${dragData.offsetX},${dragData.offsetY})`);
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });

        this._label.connect('button-release-event', (_, event) => {
            if (event.get_button() === 1) {
                console.log('Button released');
                dragData.dragging = false;
                const x = this._label.get_x();
                const y = this._label.get_y();
                console.log(`Drag end: position(${x},${y})`);
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
                // console.log(`Dragging: new position(${newX},${newY})`); // Commented out excessive logging
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });
    }

    _showMenu() {
        if (!this._menu) {
            this._menu = new PopupMenu.PopupMenu(this._label, 0.5, St.Side.TOP);
            
            // Add shutdown option
            const shutdownItem = this._menu.addAction('Shutdown', () => {
                console.log('Shutdown selected');
                try {
                    GLib.spawn_command_line_async('/usr/bin/systemctl poweroff');
                } catch (e) {
                    console.error('Failed to execute shutdown:', e);
                }
            });
            
            // Add separator
            this._menu.addMenuItem(new PopupMenu.PopupSeparatorMenuItem());
            
            // Add close menu option
            this._menu.addAction('Close', () => {
                this._menu.close();
            });
            
            Main.uiGroup.add_child(this._menu.actor);
        }
        
        this._menu.open();
    }
    
    _sendMessageToDaemon(message) {
        try {
            if (!this._socketClient) {
                this._socketClient = new Gio.SocketClient();
            }
            let connection = this._socketClient.connect_to_uri(
                `unix://${AUTOMATE_LINUX_SOCKET_PATH}`,
                null, // cancellable
                null  // error
            );

            if (!connection) {
                throw new Error('Could not connect to daemon socket.');
            }

            let istream = connection.get_input_stream();
            let ostream = connection.get_output_stream();

            // Send message
            let output = new Gio.DataOutputStream({base_stream: ostream});
            output.put_string(message + '\n', null); // null for cancellable, error
            output.close(null);

            // Read response
            let input = new Gio.DataInputStream({base_stream: istream});
            let response = input.read_line_utf8(null)[0]; // [0] for actual string, [1] for length
            input.close(null);
            
            connection.close(null);
            return response ? response.trim() : '';

        } catch (e) {
            console.error(`Failed to communicate with daemon: ${e.message}`);
            return '';
        }
    }

    _loadPosition() {
        let x = null, y = null;
        try {
            const responseX = this._sendMessageToDaemon(`{"command":"getEntry", "key":"clockPositionX"}`);
            const responseY = this._sendMessageToDaemon(`{"command":"getEntry", "key":"clockPositionY"}`);

            if (responseX) {
                const xVal = parseInt(responseX);
                if (!isNaN(xVal)) {
                    x = xVal;
                }
            }
            if (responseY) {
                const yVal = parseInt(responseY);
                if (!isNaN(yVal)) {
                    y = yVal;
                }
            }
            
            console.log(`Loaded position: X=${x}, Y=${y}`);
            return {x, y};
        } catch (e) {
            console.log('Failed to load position:', e);
        }
        return {x: null, y: null};
    }

    _savePosition(x, y) {
        console.log(`Saving position: X=${Math.round(x)}, Y=${Math.round(y)}`);
        
        try {
            this._sendMessageToDaemon(`{"command":"upsertEntry", "key":"clockPositionX", "value":"${Math.round(x)}"}`);
            this._sendMessageToDaemon(`{"command":"upsertEntry", "key":"clockPositionY", "value":"${Math.round(y)}"}`);
        } catch (e) {
            console.warn('Failed to save position:', e);
        }
    }

    disable() {
        console.log('ClockExtension.disable() called');
        if (this._timeoutId) {
            GLib.source_remove(this._timeoutId);
            this._timeoutId = 0;
        }

        if (this._menu) {
            this._menu.close();
            this._menu.destroy();
            this._menu = null;
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

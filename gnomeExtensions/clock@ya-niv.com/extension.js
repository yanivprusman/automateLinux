'use strict';

import St from 'gi://St';
import GLib from 'gi://GLib';
import Clutter from 'gi://Clutter';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import * as PopupMenu from 'resource:///org/gnome/shell/ui/popupMenu.js';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';

import { DaemonConnector } from '../lib/daemon.js';

const DAEMON_SOCKET_PATH = '/run/automatelinux/automatelinux-daemon.sock';

export default class ClockUI extends Extension {
    constructor(metadata) {
        super(metadata);
        this.daemon = new DaemonConnector(DAEMON_SOCKET_PATH, null);
        this._label = null;
        this._menu = null;
        this._timeoutId = 0;
    }

    enable() {
        this._label = new St.Label({
            text: '00:00',
            reactive: true,
            track_hover: true,
            x_align: Clutter.ActorAlign.START,
            y_align: Clutter.ActorAlign.START,
        });

        this._label.set_position(50, 50);
        Main.uiGroup.add_child(this._label);
        this._label.show();

        this._menu = new PopupMenu.PopupMenu(this._label, 0.5, St.Side.TOP);
        Main.uiGroup.add_child(this._menu.actor);
        this._menu.actor.hide();
        this._label.menu = this._menu;

        this._menu.addMenuItem(
            this._createToggleLoggingItem('active-window-tracker')
        );

        this._label.connect('button-press-event', (_, e) => {
            if (e.get_button() === 3) {
                this._menu.toggle();
                return Clutter.EVENT_STOP;
            }
            return Clutter.EVENT_PROPAGATE;
        });

        this._updateClock();
        this._timeoutId = GLib.timeout_add_seconds(
            GLib.PRIORITY_DEFAULT,
            1,
            () => {
                this._updateClock();
                return GLib.SOURCE_CONTINUE;
            }
        );
    }

    disable() {
        if (this._timeoutId)
            GLib.source_remove(this._timeoutId);

        this._menu?.destroy();
        this._label?.destroy();
    }

    _updateClock() {
        this._label.text = new Date().toLocaleTimeString([], {
            hour: '2-digit',
            minute: '2-digit',
            hour12: false,
        });
    }

    _createToggleLoggingItem(extensionId) {
        const item = new PopupMenu.PopupMenuItem('Enable Logging');

        item.connect('activate', async () => {
            const state = await this.daemon.connectAndSendMessage({
                command: 'toggleLogging',
                extension: extensionId,
            });

            item.label.text =
                state === 'true'
                    ? 'Disable Logging'
                    : 'Enable Logging';
        });

        return item;
    }
}

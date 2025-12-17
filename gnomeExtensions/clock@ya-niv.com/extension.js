'use strict';

import St from 'gi://St';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';

export default class ClockExtension extends Extension {
    constructor(metadata) {
        super(metadata);
        this._label = null;
    }

    enable() {
        console.log('ClockExtension.enable() called - minimal');
        try {
            this._label = new St.Label({ text: 'Hello' });
            Main.uiGroup.add_child(this._label);
            this._label.set_position(50, 50);
            this._label.show();
            console.log('Static label shown');
        } catch (e) {
            console.error('Error in minimal enable():', e);
        }
    }

    disable() {
        console.log('ClockExtension.disable() called - minimal');
        if (this._label) {
            this._label.destroy();
            this._label = null;
        }
    }
}

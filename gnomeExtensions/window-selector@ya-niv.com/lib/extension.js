import * as Main from 'resource:///org/gnome/shell/ui/main.js';

export default class Extension {
    constructor() {
        // Constructor - called when extension is loaded
    }

    enable() {
        // Called when extension is enabled
        log('Extension enabled');
    }

    disable() {
        // Called when extension is disabled
        log('Extension disabled');
    }
}
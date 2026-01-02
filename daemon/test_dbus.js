#!/usr/bin/gjs
const { Gio, GLib } = imports.gi;

console.log('Listening for system bus signal...');
const loop = new GLib.MainLoop(null, false);

Gio.DBus.system.signal_subscribe(
    null, // sender
    'com.automatelinux.daemon', // interface
    'Ready', // member
    '/com/automatelinux/daemon', // object path
    null, // arg0
    Gio.DBusSignalFlags.NONE,
    () => {
        console.log('Daemon ready signal received via System DBus!');
        loop.quit();
    }
);

GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 10, () => {
    console.log('Timed out waiting for signal');
    loop.quit();
    return GLib.SOURCE_REMOVE;
});

loop.run();

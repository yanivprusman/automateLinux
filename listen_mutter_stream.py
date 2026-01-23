import gi
gi.require_version('Gio', '2.0')
from gi.repository import Gio, GLib

def on_stream_added(connection, sender_name, object_path, interface_name, signal_name, parameters, user_data):
    node_id = parameters.unpack()[0]
    print(f"\n[SIGNAL] PipeWireStreamAdded! Node ID: {node_id}")
    print("SUCCESS! We found the Node ID.")
    loop.quit()

connection = Gio.bus_get_sync(Gio.BusType.SESSION, None)

# 1. Create Session
print("Creating Session...")
ret = connection.call_sync(
    "org.gnome.Mutter.ScreenCast",
    "/org/gnome/Mutter/ScreenCast",
    "org.gnome.Mutter.ScreenCast",
    "CreateSession",
    GLib.Variant("(a{sv})", ({},)),
    None,
    Gio.DBusCallFlags.NONE,
    -1,
    None
)
session_path = ret[0]
print(f"Session: {session_path}")

# 2. Record Monitor
print("Recording Monitor...")
ret_stream = connection.call_sync(
    "org.gnome.Mutter.ScreenCast",
    session_path,
    "org.gnome.Mutter.ScreenCast.Session",
    "RecordMonitor",
    GLib.Variant("(sa{sv})", ("", {})),
    None,
    Gio.DBusCallFlags.NONE,
    -1,
    None
)
stream_path = ret_stream[0]
print(f"Stream: {stream_path}")

# 3. Subscribe to Signal on Stream Path
# Note: Signal "PipeWireStreamAdded" on interface "org.gnome.Mutter.ScreenCast.Stream"
subscription_id = connection.signal_subscribe(
    "org.gnome.Mutter.ScreenCast",
    "org.gnome.Mutter.ScreenCast.Stream",
    "PipeWireStreamAdded",
    stream_path,
    None,
    Gio.DBusSignalFlags.NONE,
    on_stream_added,
    None
)

# 4. Start
print("Starting...")
connection.call_sync(
    "org.gnome.Mutter.ScreenCast",
    session_path,
    "org.gnome.Mutter.ScreenCast.Session",
    "Start",
    None,
    None,
    Gio.DBusCallFlags.NONE,
    -1,
    None
)

# 5. Loop
print("Waiting for signal...")
loop = GLib.MainLoop()
try:
    loop.run()
except KeyboardInterrupt:
    pass

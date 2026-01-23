import gi
gi.require_version('Gio', '2.0')
from gi.repository import Gio, GLib

def probe(bus, object_path, interface, method):
    try:
        bus.call_sync(
            "org.gnome.Mutter.ScreenCast",
            object_path,
            interface,
            method,
            None, # params
            None,
            Gio.DBusCallFlags.NONE,
            -1,
            None
        )
        print(f"[SUCCESS] {method} exists on {interface}")
    except Exception as e:
        print(f"[FAIL] {method}: {e}")

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
print(f"Session Path: {session_path}")

# 2. Probe Session Methods
print("\nProbing Session Methods:")
try:
    print("Calling RecordMonitor('', {})...")
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
    print(f"[SUCCESS] RecordMonitor returned: {stream_path}")
    
    # Try Start
    print("Calling Start on Session...")
    ret_start = connection.call_sync(
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
    print(f"[SUCCESS] Start called. Return: {ret_start}")
    
    # Introspect Session Properties
    print(f"Reading properties from Session {session_path}...")
    sess_props = connection.call_sync(
        "org.gnome.Mutter.ScreenCast",
        session_path,
        "org.freedesktop.DBus.Properties",
        "GetAll",
        GLib.Variant("(s)", ("org.gnome.Mutter.ScreenCast.Session",)),
        None,
        Gio.DBusCallFlags.NONE,
        -1,
        None
    )
    print(f"Session Props: {sess_props}")

    
    # Introspect Stream Properties
    print(f"Reading properties from {stream_path}...")
    props = connection.call_sync(
        "org.gnome.Mutter.ScreenCast",
        stream_path,
        "org.freedesktop.DBus.Properties",
        "GetAll",
        GLib.Variant("(s)", ("org.gnome.Mutter.ScreenCast.Stream",)),
        None,
        Gio.DBusCallFlags.NONE,
        -1,
        None
    )
    print(f"Stream Props: {props}")

    
except Exception as e:
    print(f"[FAIL] RecordMonitor sequence: {e}")


# 3. Try RemoteDesktop
print("\n--- RemoteDesktop ---")
ret_rd = connection.call_sync(
    "org.gnome.Mutter.RemoteDesktop",
    "/org/gnome/Mutter/RemoteDesktop",
    "org.gnome.Mutter.RemoteDesktop",
    "CreateSession",
    None,
    None,
    Gio.DBusCallFlags.NONE,
    -1,
    None
)
rd_session_path = ret_rd[0]
print(f"RD Session Path: {rd_session_path}")

methods_rd = ["Start", "Stop", "NotifyKeyboardKeycode", "NotifyPointerMotion"]
for m in methods_rd:
    probe(connection, rd_session_path, "org.gnome.Mutter.RemoteDesktop.Session", m)

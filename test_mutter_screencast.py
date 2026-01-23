import gi
gi.require_version('Gio', '2.0')
from gi.repository import Gio, GLib
import sys

def on_start_response(source, res, data):
    try:
        result = source.call_finish(res)
        print(f"Start success: {result}", flush=True)
        
        # Introspect result
        # data is stream_path
        stream_path = data
        print(f"Introspecting {stream_path}...", flush=True)
        
        bus.call(
            "org.gnome.Mutter.ScreenCast",
            stream_path,
            "org.freedesktop.DBus.Introspectable",
            "Introspect",
            None,
            None,
            Gio.DBusCallFlags.NONE,
            -1,
            None,
            on_introspect_response,
            None
        )
    except Exception as e:
        print(f"Start failed: {e}", flush=True)
        loop.quit()

def on_introspect_response(source, res, data):
    try:
        result = source.call_finish(res)
        xml = result.unpack()[0]
        print(f"Introspection XML:\n{xml}", flush=True)
    except Exception as e:
         print(f"Introspection failed: {e}", flush=True)
    loop.quit()

def on_record_response(source, res, data):
    try:
        result = source.call_finish(res)
        print(f"Record success: {result}", flush=True)
        # result is (path,)
        path = result.unpack()[0]
        # Now call Start (if needed? usually Start is called on ScreenCast session? 
        # Wait, Mutter API might differ from Portal.)
        # Mutter ScreenCast: 
        # CreateSession -> path
        # RecordMonitor(path, ...) -> stream_path
        # Session.Start()
        
        # Let's try to RecordMonitor
        # We need a variant for properties.
        # Actually RecordMonitor takes (dummy, options).
        # Signature: a{sv} -> o
        pass
    except Exception as e:
        print(f"Record failed: {e}", flush=True)
    loop.quit()

def on_session_created(source, res, data):
    try:
        result = source.call_finish(res)
        session_path = result.unpack()[0]
        print(f"Session Created: {session_path}", flush=True)


        # Now try to RecordMonitor
        # org.gnome.Mutter.ScreenCast.Session interface
        # Signature: (sa{sv}) -> o
        # s: connector name (empty string might mean 'all' or 'virtual' or fail?)
        # Let's try passing dictionary with 'cursor-mode': 1 (embedded)
        
        connector_name = connector
        # To be safe, let's try to catch error.
        
        bus.call(
            "org.gnome.Mutter.ScreenCast",
            session_path,
            "org.gnome.Mutter.ScreenCast.Session",
            "RecordMonitor",
            GLib.Variant("(sa{sv})", (connector_name, {"cursor-mode": GLib.Variant("u", 1)})), 
            GLib.VariantType("(o)"),
            Gio.DBusCallFlags.NONE,
            -1,
            None,
            on_record_monitor_response,
            session_path
        )

    except Exception as e:
        print(f"CreateSession failed: {e}", flush=True)
        loop.quit()

def on_record_monitor_response(source, res, session_path):
    try:
        result = source.call_finish(res)
        stream_path = result.unpack()[0]
        print(f"RecordMonitor Success. Stream Path: {stream_path}", flush=True)
        
        # Now Start the session
        bus.call(
            "org.gnome.Mutter.ScreenCast",
            session_path,
            "org.gnome.Mutter.ScreenCast.Session",
            "Start",
            None,
            None,
            Gio.DBusCallFlags.NONE,
            -1,
            None,
            on_start_response,
            stream_path
        )
    except Exception as e:
        print(f"RecordMonitor failed: {e}", flush=True)
        loop.quit()


loop = GLib.MainLoop()
bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)

def get_connector():
    try:
        ret = bus.call_sync(
            "org.gnome.Mutter.DisplayConfig",
            "/org/gnome/Mutter/DisplayConfig",
            "org.gnome.Mutter.DisplayConfig",
            "GetCurrentState",
            None,
            None, # Relax request type
            Gio.DBusCallFlags.NONE,
            -1,
            None
        )
        # The 7th element is monitors: a(siidda{sv})
        # (serial, u1, b1, b2, crtcs, outputs, modes, max_screen_size, global_scale)
        # outputs: (connector_info, monitor_info...)
        # monitors: s:connector, i:vendor, i:product, d:serial, d:?, a{sv}:props
        # Actually structure is complex.
        # Let's verify return type via busctl output if possible. 
        # But based on memory:
        # GetCurrentState -> (u serial, b sw_cursor, b sw_rotate, b sw_scale, a(...) crtcs, a(...) outputs, a(...) modes, ...)
        # monitors match outputs via crtc assignment. 
        # But simpler: just inspect 'monitors' arrays in the return structure if I can parse it.
        # Let's unpack assuming standard signature.
        
        # Unpack: (u_serial, b, b, b, crtcs, outputs, modes, max_w, max_h) ?? No.
        # Let's inspect signature from result.
        
        # Actually, simpler: Use 'RecordWindow' with an empty window (cursor?) No.
        # Use 'RecordVirtual' to avoid connector?
        # But we want to capture the screen.
        
        # Let's try to pass "" as connector.
        return ret
    except Exception as e:
        print(f"Failed to get config: {e}")
        return None


# Valid connector found: DP-1
connector = "DP-1" 

print("Calling CreateSession on org.gnome.Mutter.ScreenCast...", flush=True)

options = {}



bus.call(
    "org.gnome.Mutter.ScreenCast",
    "/org/gnome/Mutter/ScreenCast",
    "org.gnome.Mutter.ScreenCast",
    "CreateSession",
    GLib.Variant("(a{sv})", (options,)),
    GLib.VariantType("(o)"),
    Gio.DBusCallFlags.NONE,
    -1,
    None,
    on_session_created,
    None
)

loop.run()


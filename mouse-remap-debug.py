# #!/usr/bin/env python3
# import evdev
# from evdev import UInput, ecodes as e
# import sys

# def main():
#     device_path = '/dev/input/event6'
    
#     try:
#         device = evdev.InputDevice(device_path)
#         print(f"Device: {device.name}")
#         print(f"Path: {device.path}")
#     except Exception as ex:
#         print(f"Error opening device: {ex}")
#         sys.exit(1)
    
#     device.grab()
    
#     # Virtual mouse
#     mouse_cap = {
#         e.EV_KEY: [e.BTN_LEFT, e.BTN_RIGHT, e.BTN_MIDDLE, e.BTN_SIDE, e.BTN_EXTRA, e.BTN_BACK, e.BTN_TASK],
#         e.EV_REL: [e.REL_X, e.REL_Y, e.REL_WHEEL, e.REL_HWHEEL],
#         e.EV_MSC: [e.MSC_SCAN],
#     }
#     virtual_mouse = UInput(mouse_cap, name='Virtual Mouse')
    
#     # Virtual keyboard
#     virtual_keyboard = UInput()
    
#     print("Remapper started. Press your extra mouse button...")
#     print(f"Looking for BTN_FORWARD (code {e.BTN_FORWARD})")
    
#     try:
#         for event in device.read_loop():
#             # Debug: print all key events
#             if event.type == e.EV_KEY:
#                 print(f"KEY EVENT: code={event.code}, value={event.value}")
            
#             if event.type == e.EV_KEY and event.code == e.BTN_FORWARD:
#                 print(f"*** BTN_FORWARD DETECTED! value={event.value} ***")
#                 if event.value == 1:
#                     print("Sending KEY_ENTER press")
#                     virtual_keyboard.write(e.EV_KEY, e.KEY_ENTER, 1)
#                     virtual_keyboard.syn()
#                 elif event.value == 0:
#                     print("Sending KEY_ENTER release")
#                     virtual_keyboard.write(e.EV_KEY, e.KEY_ENTER, 0)
#                     virtual_keyboard.syn()
#             else:
#                 virtual_mouse.write_event(event)
                
#     except KeyboardInterrupt:
#         print("\nStopping...")
#     finally:
#         device.ungrab()
#         virtual_mouse.close()
#         virtual_keyboard.close()

# if __name__ == "__main__":
#     # main()
#     pass
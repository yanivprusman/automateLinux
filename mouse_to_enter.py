#!/usr/bin/env python3
import evdev
import uinput
import time

# Path to your mouse device (update if needed)
MOUSE_DEVICE = '/dev/input/event6'
BTN_FORWARD = 277  # evtest code for BTN_FORWARD

# Open mouse device
mouse = evdev.InputDevice(MOUSE_DEVICE)

# with uinput.Device([uinput.KEY_1]) as ui:
#     print('Listening for BTN_FORWARD events...')
#     for event in mouse.read_loop():
#         if event.type == evdev.ecodes.EV_KEY and event.code == BTN_FORWARD:
#             if event.value == 1:  # Key press
#                 ui.emit(uinput.KEY_1, 1)  # Press Enter
#                 ui.emit(uinput.KEY_1, 0)  # Press Enter
#                 time.sleep(0.05)  
#                 ui.emit(uinput.KEY_1, 1)  # Press Enter
#                 ui.emit(uinput.KEY_1, 0)  # Press Enter
#                 time.sleep(0.05)  
#                 ui.emit(uinput.KEY_1, 1)  # Press Enter
#                 ui.emit(uinput.KEY_1, 0)  # Press Enter
#                 time.sleep(0.05)
#                 ui.emit(uinput.KEY_1, 1)  # Press Enter
#                 ui.emit(uinput.KEY_1, 0)  # Press Enter
#             elif event.value == 0:  # Key release
#                 ui.emit(uinput.KEY_1, 1)  # Press Enter
#                 ui.emit(uinput.KEY_1, 0)  # Press Enter
#                 time.sleep(0.05)
#                 ui.emit(uinput.KEY_1, 1)  # Press Enter
#                 ui.emit(uinput.KEY_1, 0)  # Press Enter
#                 time.sleep(0.05)
#                 ui.emit(uinput.KEY_1, 1)  # Press Enter
#                 ui.emit(uinput.KEY_1, 0)  # Press Enter
#                 time.sleep(0.05)
#                 ui.emit(uinput.KEY_1, 1)  # Press Enter
#                 ui.emit(uinput.KEY_1, 0)  # Press Enter
#         time.sleep(0.001)
# print (uinput.KEY_1)
# print (uinput.KEY_ENTER)

# with uinput.Device([uinput.KEY_1]) as ui:
#     for event in mouse.read_loop():
#         if event.type == evdev.ecodes.EV_KEY and event.code == BTN_FORWARD:
#             print("Asdf")
#         else:
#             print("AAAA")

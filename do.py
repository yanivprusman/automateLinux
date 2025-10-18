import uinput, time

device = uinput.Device([uinput.KEY_1, uinput.KEY_2])

# device.emit_click(uinput.KEY_1)
# device.emit_click(uinput.KEY_2)
while True:
    input("Press Enter to emit 12: ")
    device.emit_click(uinput.KEY_1)
    device.emit_click(uinput.KEY_2)
#!/usr/bin/env python3
from evdev import UInput, ecodes as e

capabilities = {
    e.EV_KEY: [
        e.KEY_L, e.KEY_S, e.KEY_ENTER
    ]
}

with UInput(capabilities, name="synthetic-keyboard", bustype=0x03) as ui:
    ui.write(e.EV_KEY, e.KEY_L, 1)
    ui.write(e.EV_KEY, e.KEY_L, 0)
    ui.write(e.EV_KEY, e.KEY_S, 1)
    ui.write(e.EV_KEY, e.KEY_S, 0)
    ui.write(e.EV_KEY, e.KEY_ENTER, 1)
    ui.write(e.EV_KEY, e.KEY_ENTER, 0)
    ui.syn()

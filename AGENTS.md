help me map extra mouse key to enter
on wayland
here is what i got:
yaniv@ubuntu:~/coding/FreeCADFork$ xev | grep button
   state 0x0, button 10, same_screen YES
yaniv@ubuntu:~/coding/FreeCADFork$ sudo evtest /dev/input/event6
Input driver version is 1.0.1
Input device ID: bus 0x3 vendor 0x46d product 0xc548 version 0x111
Input device name: "Logitech USB Receiver Mouse"
Supported events:
  Event type 0 (EV_SYN)
  Event type 1 (EV_KEY)
    Event code 272 (BTN_LEFT)
    Event code 273 (BTN_RIGHT)
    Event code 274 (BTN_MIDDLE)
    Event code 275 (BTN_SIDE)
    Event code 276 (BTN_EXTRA)
    Event code 277 (BTN_FORWARD)
    Event code 278 (BTN_BACK)
    Event code 279 (BTN_TASK)
    Event code 280 (?)
    Event code 281 (?)
    Event code 282 (?)
    Event code 283 (?)
    Event code 284 (?)
    Event code 285 (?)
    Event code 286 (?)
    Event code 287 (?)
  Event type 2 (EV_REL)
    Event code 0 (REL_X)
    Event code 1 (REL_Y)
    Event code 6 (REL_HWHEEL)
    Event code 8 (REL_WHEEL)
    Event code 11 (REL_WHEEL_HI_RES)
    Event code 12 (REL_HWHEEL_HI_RES)
  Event type 4 (EV_MSC)
    Event code 4 (MSC_SCAN)
Properties:
Testing ... (interrupt to exit)
Event: time 1760701091.617689, type 4 (EV_MSC), code 4 (MSC_SCAN), value 90006
Event: time 1760701091.617689, type 1 (EV_KEY), code 277 (BTN_FORWARD), value 1
Event: time 1760701091.617689, -------------- SYN_REPORT ------------
Event: time 1760701091.895685, type 4 (EV_MSC), code 4 (MSC_SCAN), value 90006
Event: time 1760701091.895685, type 1 (EV_KEY), code 277 (BTN_FORWARD), value 0
Event: time 1760701091.895685, -------------- SYN_REPORT ------------

i think the key code of the extra button is 28

look at /etc/systemd/system/mouse-remap.service
look at /usr/local/bin/mouse-remap.py
when i do:
yaniv@ubuntu:~/coding/FreeCADFork$ sudo evtest /dev/input/event6
Input driver version is 1.0.1
Input device ID: bus 0x3 vendor 0x46d product 0xc548 version 0x111
Input device name: "Logitech USB Receiver Mouse"
Supported events:
  Event type 0 (EV_SYN)
  Event type 1 (EV_KEY)
    Event code 28 (KEY_ENTER)
    Event code 272 (BTN_LEFT)
    Event code 273 (BTN_RIGHT)
    Event code 274 (BTN_MIDDLE)
    Event code 275 (BTN_SIDE)
    Event code 276 (BTN_EXTRA)
    Event code 277 (BTN_FORWARD)
    Event code 278 (BTN_BACK)
    Event code 279 (BTN_TASK)
    Event code 280 (?)
    Event code 281 (?)
    Event code 282 (?)
    Event code 283 (?)
    Event code 284 (?)
    Event code 285 (?)
    Event code 286 (?)
    Event code 287 (?)
  Event type 2 (EV_REL)
    Event code 0 (REL_X)
    Event code 1 (REL_Y)
    Event code 6 (REL_HWHEEL)
    Event code 8 (REL_WHEEL)
    Event code 11 (REL_WHEEL_HI_RES)
    Event code 12 (REL_HWHEEL_HI_RES)
  Event type 4 (EV_MSC)
    Event code 4 (MSC_SCAN)
Properties:
Testing ... (interrupt to exit)
***********************************************
  This device is grabbed by another process.
  No events are available to evtest while the
  other grab is active.
  In most cases, this is caused by an X driver,
  try VT-switching and re-run evtest again.
  Run the following command to see processes with
  an open fd on this device
 "fuser -v /dev/input/event6"
***********************************************
i checked:
yaniv@ubuntu:~/coding/FreeCADFork$ fuser -v /dev/input/event6
                     USER        PID ACCESS COMMAND
/dev/input/event6:   yaniv      2683 F.... gnome-shell



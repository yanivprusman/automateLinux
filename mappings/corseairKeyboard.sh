# KEYBOARD_BY_ID=usb-Corsair_CORSAIR_K100_RGB_Optical-Mechanical_Gaming_Keyboard_502A81D24AAA7CC6-event-kbd
KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
sudo evsieve --input /dev/input/by-id/$KEYBOARD_BY_ID  \
    --hook key:leftctrl key:h exec-shell="echo Hello, world!" \

sudo evsieve --input /dev/input/by-id/$KEYBOARD_BY_ID grab \
    --map key:f2 key:leftctrl:0 key:s:0 key:s:1 key:leftctrl:1 \
    --map key:f3 key:leftctrl:0 key:y:0 key:y:1 key:leftctrl:1 \
    --map key:f4 key:leftctrl:0 key:z:0 key:z:1 key:leftctrl:1 \
    --output
 


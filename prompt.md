in /home/yaniv/coding/automateLinux/daemon/src/mainCommand.cpp
instead of doing
    string cmd = string(
        "sudo systemctl stop corsairKeyBoardLogiMouse 2>&1 ; "
        "sudo systemd-run --collect --service-type=notify --unit=corsairKeyBoardLogiMouse.service "
        "--property=StandardError=append:" + directories.data + EVSIEVE_STANDARD_ERR_FILE + " "
        "--property=StandardOutput=append:" + directories.data + EVSIEVE_STANDARD_OUTPUT_FILE + " "
        )
        + scriptContent
    ;

can i have a bash process in the background that runs evsieve and stops it and runs it again with the changed keyboard?
will it be faster?

i have added a symlink to an extraApp loom make sure it has a port fot dev and a port for prod. add 
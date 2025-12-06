echo "Caught SIGHUP signal: terminal hangup">/tmp/test.txt
echo "FD_IN=$AUTOMATE_LINUX_DAEMON_FD_IN" >> /tmp/test.txt
echo "FD_OUT=$AUTOMATE_LINUX_DAEMON_FD_OUT" >> /tmp/test.txt
# type daemon >> /tmp/test.txt 2>&1
if ! test -e /proc/$$/fd/$AUTOMATE_LINUX_DAEMON_FD_IN; then
    echo "FD_IN invalid" >> /tmp/test.txt
fi
if ! test -e /proc/$$/fd/$AUTOMATE_LINUX_DAEMON_FD_OUT; then
    echo "FD_OUT invalid" >> /tmp/test.txt
fi
daemon closedTty

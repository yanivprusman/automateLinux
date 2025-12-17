(return 0 2>/dev/null) || { echo "Script must be sourced"; exit 1; }
if [[ " $@ " =~ " -rebuild " ]]; then
    rm -rf build
fi
if [ ! -d "build" ]; then
    mkdir -p build
fi
cd build
cmake .. > /dev/null && \
make > /dev/null && \
echo -e "${GREEN}Build complete!${NC}" && \
# if [[ -n "$DAEMON_COPROC_PID" ]]; then
#     kill $DAEMON_COPROC_PID 2>/dev/null
#     wait $DAEMON_COPROC_PID 2>/dev/null
#     DAEMON_COPROC_PID=
# fi && \
sudo systemctl restart daemon.service && \
sleep 0.5 && \
AUTOMATE_LINUX_DAEMON_PID=$(${AUTOMATE_LINUX_DAEMON_DIR}getDaemonPID.sh) && \
coproc DAEMON_COPROC { socat - UNIX-CONNECT:"$AUTOMATE_LINUX_SOCKET_PATH" 2>/dev/null; } && \
export AUTOMATE_LINUX_DAEMON_FD_IN=${DAEMON_COPROC[1]} && \
export AUTOMATE_LINUX_DAEMON_FD_OUT=${DAEMON_COPROC[0]} 
cd ..

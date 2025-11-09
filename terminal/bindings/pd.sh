pd
# ${PROMPT_COMMAND:-:}
# bind -x '"\e[1;5A":". $AUTOMATE_LINUX_BINDINGS_DIR/pd.sh; printf \"\r\033[K\"; ${PROMPT_COMMAND:-:}"'
# sudo python3 "${AUTOMATE_LINUX_BINDINGS_DIR}injectEnter.py"
evsieve \
    --output name="created by evsieve" create-link=/dev/input/by-id/createdByEvsieve repeat=disable




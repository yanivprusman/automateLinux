# Gemini related utilities

geminiUpsertSession() {
    local FILE="${AUTOMATE_LINUX_DIR:-/opt/automateLinux}/data/geminiSessions.sh"
    local KEY="$1"
    local VALUE="$2"

    if [ -z "$KEY" ] || [ -z "$VALUE" ]; then
        echo "Usage: geminiUpsertSession <key> <value>"
        return 1
    fi

    # Check if the key already exists and update it
    if grep -q "^$KEY=" "$FILE"; then
        # Key exists, update the value
        sed -i "s/^$KEY=.*/$KEY=$VALUE/" "$FILE"
        echo "Updated $KEY=$VALUE"
    else
        # Key does not exist, append it
        echo "$KEY=$VALUE" >> "$FILE"
        echo "Added $KEY=$VALUE"
    fi
    # Set the environment variable in the current session
    export "$KEY=$VALUE"
}
export -f geminiUpsertSession

geminiCleanBrain() {
    local target_dir="${HOME}/.gemini/antigravity/brain"
    local conversations_dir="${HOME}/.gemini/antigravity/conversations"

    if [ -d "$target_dir" ]; then
        if [ "$(ls -A "$target_dir")" ]; then
            rm -rf "${target_dir:?}"/*
            echo "Cleaned ${target_dir}"
        else
            echo "${target_dir} is already empty."
        fi
    else
        echo "Directory ${target_dir} does not exist."
    fi

    if [ -d "$conversations_dir" ]; then
        if [ "$(ls -A "$conversations_dir")" ]; then
            rm -rf "${conversations_dir:?}"/*
            echo "Cleaned ${conversations_dir}"
        else
            echo "${conversations_dir} is already empty."
        fi
    else
        echo "Directory ${conversations_dir} does not exist."
    fi
}
export -f geminiCleanBrain

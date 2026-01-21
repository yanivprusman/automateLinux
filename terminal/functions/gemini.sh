# Gemini related utilities

geminiUpsertSession() {
    local FILE="/home/yaniv/coding/automateLinux/data/geminiSessions.sh"
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
    if [ -d "$target_dir" ]; then
        rm -rf "${target_dir:?}"/*
        echo "Cleaned ${target_dir}"
    else
        echo "Directory ${target_dir} does not exist."
    fi
}
export -f geminiCleanBrain

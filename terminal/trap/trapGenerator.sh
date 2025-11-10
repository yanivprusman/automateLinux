generateIntentionalError() {
    local exit_code=$1
    echo "Generating intentional error with exit code: $exit_code"
    return $exit_code
}
set -x
generateIntentionalError 127
set +x

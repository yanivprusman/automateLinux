in bash i want to create one command that sources a file (in the path) and executes a function in it.
the name of the file and function are theRealPathFile and theRealPathFunction respectively.
two alternatives alias or function
tel me the pros and cons of both.
this it the contents of file theRealPathFile:
#!/bin/bash
if [ -z "$BASH_VERSION" ]; then
    echo "This script requires bash" >&2
    return 1 2>/dev/null || exit 1
fi
theRealPathFunction() {
    realpath "$(dirname "$(realpath "${BASH_SOURCE[-1]}")")/$1"
}
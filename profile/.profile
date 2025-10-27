# ~/.profile: executed by the command interpreter for login shells.

# if running bash
# if [ -n "$BASH_VERSION" ]; then
    if [ -f "$HOME/.bashrc" ]; then
        . "$HOME/.bashrc"
    fi
# fi

# set PATH so it includes user's private bins if they exist
for dir in "$HOME/bin" "$HOME/.local/bin" "$HOME/.npm-global/bin" "$HOME/go/bin" "$HOME/.pixi/bin"; do
    if [ -d "$dir" ]; then
        case ":$PATH:" in
            *":$dir:"*) :;; # already there
            *) PATH="$dir:$PATH";;
        esac
    fi
done

# Rust
[ -f "$HOME/.cargo/env" ] && . "$HOME/.cargo/env"

# Python
export PYENV_ROOT="$HOME/.pyenv"
if [ -d "$PYENV_ROOT/bin" ]; then
    PATH="$PYENV_ROOT/bin:$PATH"
    eval "$(pyenv init --path)"
fi

# FreeCAD
export FREECAD_MACROS_DIR="/home/yaniv/101_coding/freeCad/Macros/"
export PYTHONPATH="/home/yaniv/freecad/FreeCADApp/squashfs-root/usr/lib/python3.11/site-packages:$PYTHONPATH"

# Compiler path
export COMPILER_PATH="/usr/libexec/gcc/x86_64-linux-gnu/13"
PATH="$COMPILER_PATH:$PATH"

export PATH


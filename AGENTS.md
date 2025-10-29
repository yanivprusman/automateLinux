in 
/home/yaniv/coding/automateLinux/bashrc/aliases.sh
help me make a function or alias to replace:
theRealPath x
with
$(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/x")
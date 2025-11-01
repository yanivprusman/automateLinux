write a script to simplify 
sudo $(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/_sudoStop.sh")
it can be sourced by design or not sourced by design (help me decide)
inside the script it will call the same sequence of:
    ${BASH_SOURCE[0]}
    realpath
    dirname
    realpath
    and add the file name that is given.
    the file name can be a relative file name (like it works in the $(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/x") )  syntax)
    

in the end the user will call theRealPath (may be sourced beforehand) and append the relative path :
theRealPath path/to/file/relative/to/this/file
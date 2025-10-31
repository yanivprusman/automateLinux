# 2) calling from terminal a script in current directory that sudo calls a target script in a different directory
echo "in theLongWay2.sh"
echo "realpath BASH_SOURCE[0]: $(realpath "${BASH_SOURCE[0]}")"
sudo $(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/dir1/echo.sh")

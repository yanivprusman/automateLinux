# 1) calling from terminal a script in current directory that sudo calls a target script in the same directory
echo "in theLongWay1.sh"
echo "realpath BASH_SOURCE[0]: $(realpath "${BASH_SOURCE[0]}")"
sudo $(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/echo.sh")

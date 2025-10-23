sudo systemctl stop keyBoardTest.service 2>/dev/null
sudo systemctl reset-failed keyBoardTest.service 2>/dev/null
echo "$(dirname "${BASH_SOURCE[0]}")./test.sh"
# sudo "$(dirname "${BASH_SOURCE[0]}")./test.sh"
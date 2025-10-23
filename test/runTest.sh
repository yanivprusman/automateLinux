sudo systemctl stop keyBoardTest.service 2>/dev/null
sudo systemctl reset-failed keyBoardTest.service 2>/dev/null
sudo ./test.sh
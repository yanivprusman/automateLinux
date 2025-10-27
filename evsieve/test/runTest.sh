#!/usr/bin/env bash
sudo systemctl stop keyBoardMouseTest.service 2>/dev/null
sudo systemctl reset-failed keyBoardMouseTest.service 2>/dev/null
sudo "$(dirname "${BASH_SOURCE[0]}")/test.sh"
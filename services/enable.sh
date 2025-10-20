sudo ln -s /home/yaniv/coding/automateLinux/services/run-mappings.service /etc/systemd/system/run-mappings.service
sudo systemctl restart run-mappings.service
sudo systemctl disable run-mappings.service


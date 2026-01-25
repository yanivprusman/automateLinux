# enableChromeAsRoot(){
#     useradd chrome -s /bin/bash -d /home/chrome -m
# }
chromeAsRoot(){
    google-chrome --no-sandbox --user-data-dir=/root/chrome
}
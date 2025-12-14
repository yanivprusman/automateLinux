# sourced from terminal
cd /home/yaniv/coding
theRealPath
# [[$(theRealPath) == /home/yaniv/coding/]] && echo TRUE
# theRealPath . expected /home/yaniv/coding/
# theRealPath ./ expected /home/yaniv/coding/
# theRealPath asdf expected /home/yaniv/coding/asdf
# theRealPath ./asdf expected /home/yaniv/coding/asdf
# theRealPath ../asdf expected /home/yaniv/asdf
cd -
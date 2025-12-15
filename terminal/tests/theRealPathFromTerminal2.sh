#                                                                   called in terminal  | sourced from terminal
cd /home/yaniv/coding
theRealPath -debug                                            #      /home/yaniv/coding/ | /home/yaniv/coding/theRealPathFromTerminal.sh (the name of this test script)
# [[$(theRealPath) == /home/yaniv/coding/]] && echo TRUE
# theRealPath . expected /home/yaniv/coding/
# theRealPath ./ expected /home/yaniv/coding/
# theRealPath asdf expected /home/yaniv/coding/asdf
# theRealPath ./asdf expected /home/yaniv/coding/asdf
# theRealPath ../asdf expected /home/yaniv/asdf
cd - > /dev/null
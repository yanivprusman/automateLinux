when on chrome and the current tab url starts with https://chatgpt.com and the user presses leftctrl+v we need to focus on the text box and paste.
there needs to be a communication via Native messaging chrome - daemon
the user needs to paste once and not paste once for focusing and another paste for pasting.
we need to automate each step of the process of testing this:
1) reloading the extension 
2) going to a chatgpt tab and /or creating a new tab.
3) focusing on a place other than the text box
4) simulate leftctrl+v
5) verify focus happened
6) verify paste happened.
7) make sure super speedy execution
8) simulate clicking "verify you are human" if needed etc.

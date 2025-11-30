inside classes and functions no empty lines.
do not add comments, but do not delete existing comments.

i am starting from scratch.
create a class Terminal
expect calls to daemon from:
bashrc: daemon ttyOpened called each time a terminal is opened (called once)
bashrc: daemon cdToPointer returns a string "cd " + the calculated directory
promptCommand: daemon updateDirHistory called before printing $PS1 updates the database
pd: daemon cdBackword called whenever the user wants to cd to previous dir in history
pdd: daemon cdForword called whenever the user wants to cd to next dir in history
daemon ttyClosed called when the terminal is called

for the database we will be using sqlite

the terminal will track all terminal instances
the terminal will track for each tty its history ( dir history) and pointer to current dir in that history.
when ttyOpened is called the history of that tty will be copied from the last chaged tty history tracking ( could be the same as this tty then no copying is done) includijng copying the pointer.



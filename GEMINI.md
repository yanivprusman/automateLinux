look at /home/yaniv/coding/automateLinux/daemon/src/main.cpp i am refactoring 
i want to use the daemon as a client too.
i want the version in /home/yaniv/coding/automateLinux/terminal/functions/daemon.sh to be obsolete ialready commentsed it out.
currently daemon expects a json string.
lets change that to more convenient
lets refactor /home/yaniv/coding/automateLinux/terminal/completions/daemon.bash too
i want commads like:
daemon send getDirHistory 
to work.
make the .bash aware of the context and autocomplete the new way (not key= but rather -key theKey) 

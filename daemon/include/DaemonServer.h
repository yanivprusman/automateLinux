#ifndef DAEMON_SERVER_H
#define DAEMON_SERVER_H

void signal_handler(int sig);
int initialize_daemon();
void daemon_loop();

#endif // DAEMON_SERVER_H

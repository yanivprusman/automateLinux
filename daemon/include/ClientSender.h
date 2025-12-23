#ifndef CLIENT_SENDER_H
#define CLIENT_SENDER_H

#include "using.h"

int send_command_to_daemon(const ordered_json &jsonCmd);
ordered_json parse_client_args(int argc, char *argv[], int start_index);

#endif // CLIENT_SENDER_H

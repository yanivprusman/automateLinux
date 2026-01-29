#ifndef CMD_PORT_H
#define CMD_PORT_H

#include "Types.h"
#include <json/json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Port Management Command Handlers
CmdResult handleGetPort(const json &command);
CmdResult handleSetPort(const json &command);
CmdResult handleDeletePort(const json &command);
CmdResult handleListPorts(const json &command);

// Public Transportation Commands (uses ports)
CmdResult handlePublicTransportationStartProxy(const json &command);
CmdResult handlePublicTransportationOpenApp(const json &command);

#endif // CMD_PORT_H

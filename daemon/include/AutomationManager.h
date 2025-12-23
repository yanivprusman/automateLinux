#ifndef AUTOMATION_MANAGER_H
#define AUTOMATION_MANAGER_H

#include "Types.h"
#include "using.h"
#include <jsoncpp/json/json.h>
#include <string>

class AutomationManager {
public:
  static CmdResult onActiveWindowChanged(const json &command);

private:
  static std::string getCurrentTabUrl();
};

#endif // AUTOMATION_MANAGER_H

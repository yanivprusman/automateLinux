#ifndef KEYBOARD_MANAGER_H
#define KEYBOARD_MANAGER_H

#include "InputMapper.h"
#include "Types.h"
#include <string>
#include <vector>

class KeyboardManager {
public:
  static CmdResult setKeyboard(bool enableKeyboard);
  static bool isKnownKeyboard(const std::string &name);
  static void setContext(AppType appType, const std::string &url = "",
                         const std::string &title = "");
  static void onFocusAck();

private:
  static const std::vector<std::string> KNOWN_KEYBOARDS;
  static InputMapper mapper;
};

#endif // KEYBOARD_MANAGER_H

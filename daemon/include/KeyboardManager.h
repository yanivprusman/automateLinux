#ifndef KEYBOARD_MANAGER_H
#define KEYBOARD_MANAGER_H

#include "Types.h"
#include <string>
#include <vector>

class KeyboardManager {
public:
  static CmdResult setKeyboard(const std::string &keyboardName,
                               bool toggleKeyboardsWhenActiveWindowChanges);

private:
  static const std::vector<std::string> KNOWN_KEYBOARDS;
};

#endif // KEYBOARD_MANAGER_H

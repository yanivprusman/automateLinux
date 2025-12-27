#include "InputMapper.h"
#include "Types.h"
#include <linux/input-event-codes.h>

void InputMapper::initializeAppMacros() {
  // === DEFAULT MAPPINGS (apply to all apps unless overridden) ===
  std::vector<KeyAction> defaultMacros;

  // Mouse: Forward Button -> Enter
  defaultMacros.push_back(KeyAction{KeyTrigger{{BTN_FORWARD}},
                                    {{KEY_ENTER, 1}, {KEY_ENTER, 0}},
                                    "Triggering mouse forward button → Enter",
                                    nullptr});

  // G5 -> Ctrl+V
  defaultMacros.push_back(
      KeyAction{KeyTrigger{{G5_VIRTUAL}},
                {{KEY_LEFTCTRL, 1}, {KEY_V, 1}, {KEY_V, 0}, {KEY_LEFTCTRL, 0}},
                "Triggering G5 Ctrl+V macro",
                nullptr});

  // === APP-SPECIFIC MAPPINGS (override defaults) ===

  // --- TERMINAL ---
  appMacros_[AppType::TERMINAL] = defaultMacros; // Start with defaults
  appMacros_[AppType::TERMINAL].push_back(
      KeyAction{KeyTrigger{{G1_VIRTUAL}},
                {{KEY_LEFTCTRL, 1},
                 {KEY_LEFTALT, 1},
                 {KEY_C, 1},
                 {KEY_C, 0},
                 {KEY_LEFTALT, 0},
                 {KEY_LEFTCTRL, 0}},
                "Triggering G1 SIGINT macro (Ctrl+Alt+C) for Gnome Terminal",
                nullptr});
  appMacros_[AppType::TERMINAL].push_back(KeyAction{
      KeyTrigger{{KEY_LEFTCTRL, BTN_LEFT}},
      {{KEY_LEFTCTRL, 0}, {KEY_5, 1}, {KEY_5, 0}},
      "Triggering Terminal Ctrl+Left Click macro (UNDO CONTROL AND 5)",
      nullptr});

  // --- CODE (VS Code) ---
  appMacros_[AppType::CODE] = defaultMacros; // Start with defaults
  appMacros_[AppType::CODE].push_back(
      KeyAction{KeyTrigger{{G2_VIRTUAL}},
                {{KEY_END, 1}, {KEY_END, 0}},
                "Triggering G2 End macro for VS Code",
                nullptr});
  appMacros_[AppType::CODE].push_back(
      KeyAction{KeyTrigger{{G6_VIRTUAL}},
                {{KEY_LEFTCTRL, 1}, {KEY_C, 1}, {KEY_C, 0}, {KEY_LEFTCTRL, 0}},
                "Triggering G6 Ctrl+C macro for VS Code",
                nullptr});

  // --- CHROME ---
  appMacros_[AppType::CHROME] = defaultMacros; // Start with defaults
  // Add Chrome ChatGPT Ctrl+V with callback
  appMacros_[AppType::CHROME].push_back(
      KeyAction{KeyTrigger{{KEY_LEFTCTRL, KEY_V}},
                {}, // No key sequence (callback is used instead)
                "Triggering ChatGPT Ctrl+V macro (Speculative Parallel Focus)",
                [this]() { this->triggerChromeChatGPTMacro(); }});
  appMacros_[AppType::CHROME].push_back(
      KeyAction{KeyTrigger{{G5_VIRTUAL}},
                {}, // No key sequence (callback is used instead)
                "Triggering ChatGPT G5 macro (Focus + Paste)",
                [this]() { this->triggerChromeChatGPTMacro(); }});

  // --- OTHER (default app type) ---
  appMacros_[AppType::OTHER] = defaultMacros; // Start with defaults
}

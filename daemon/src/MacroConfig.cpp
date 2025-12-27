#include "InputMapper.h"
#include "Types.h"
#include <linux/input-event-codes.h>

void InputMapper::initializeAppMacros() {
  // === DEFAULT MAPPINGS (apply to all apps unless overridden) ===
  std::vector<KeyAction> defaultMacros;

  // Mouse: Forward Button (press) -> Enter
  defaultMacros.push_back(KeyAction{KeyTrigger{{{BTN_FORWARD, 1}}, false},
                                    {{KEY_ENTER, 1}, {KEY_ENTER, 0}},
                                    "Triggering mouse forward button (press) → Enter",
                                    nullptr});

  // G5 (press) -> Ctrl+V
  defaultMacros.push_back(
      KeyAction{KeyTrigger{{{G5_VIRTUAL, 1}}, false},
                {{KEY_LEFTCTRL, 1}, {KEY_V, 1}, {KEY_V, 0}, {KEY_LEFTCTRL, 0}},
                "Triggering G5 (press) Ctrl+V macro",
                nullptr});

  // G6 (press) -> Ctrl+C
  defaultMacros.push_back(
      KeyAction{KeyTrigger{{{G6_VIRTUAL, 1}}, false},
                {{KEY_LEFTCTRL, 1}, {KEY_C, 1}, {KEY_C, 0}, {KEY_LEFTCTRL, 0}},
                "Triggering G6 (press) Ctrl+C macro",
                nullptr});

  // === APP-SPECIFIC MAPPINGS (override defaults) ===

  // --- TERMINAL ---
  appMacros_[AppType::TERMINAL] = defaultMacros; // Start with defaults
  appMacros_[AppType::TERMINAL].push_back(
      KeyAction{KeyTrigger{{{G1_VIRTUAL, 1}}, false},
                {{KEY_LEFTCTRL, 1},
                 {KEY_LEFTALT, 1},
                 {KEY_C, 1},
                 {KEY_C, 0},
                 {KEY_LEFTALT, 0},
                 {KEY_LEFTCTRL, 0}},
                "Triggering G1 (press) SIGINT macro (Ctrl+Alt+C) for Gnome Terminal",
                nullptr});
  // Ctrl(press) + LeftClick(press)
  appMacros_[AppType::TERMINAL].push_back(KeyAction{
      KeyTrigger{{{KEY_LEFTCTRL, 1}, {BTN_LEFT, 1}}, true},
      {{KEY_LEFTCTRL, 0}, {KEY_5, 1}, {KEY_5, 0}},
      "Triggering Terminal Ctrl+Left Click macro",
      nullptr});

  // --- CODE (VS Code) ---
  appMacros_[AppType::CODE] = defaultMacros; // Start with defaults
  appMacros_[AppType::CODE].push_back(
      KeyAction{KeyTrigger{{{G2_VIRTUAL, 1}}, false},
                {{KEY_END, 1}, {KEY_END, 0}},
                "Triggering G2 (press) End macro for VS Code",
                nullptr});

  // --- CHROME ---
  appMacros_[AppType::CHROME] = defaultMacros; // Start with defaults
  // Ctrl(press) + V(press) with callback
  appMacros_[AppType::CHROME].push_back(
      KeyAction{KeyTrigger{{{KEY_LEFTCTRL, 1}, {KEY_V, 1}}, true},
                {}, // No key sequence (callback is used instead)
                "Triggering ChatGPT Ctrl+V macro (Speculative Parallel Focus)",
                [this]() { this->triggerChromeChatGPTMacro(); }});

  // --- OTHER (default app type) ---
  appMacros_[AppType::OTHER] = defaultMacros; // Start with defaults
}
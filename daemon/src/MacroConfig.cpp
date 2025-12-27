#include "InputMapper.h"
#include "Types.h"
#include <linux/input-event-codes.h>

void InputMapper::initializeAppMacros() {
  // === DEFAULT MAPPINGS (apply to all apps unless overridden) ===
  std::vector<KeyAction> defaultMacros;

  // Mouse: Forward Button -> Enter
  defaultMacros.push_back(KeyAction{
      KeyTrigger{G_NONE, BTN_FORWARD, 0, ""},  // keyCode=BTN_FORWARD, no modifiers
      {{KEY_ENTER, 1}, {KEY_ENTER, 0}},
      "Triggering mouse forward button → Enter",
      nullptr});

  // G5 -> Ctrl+V
  defaultMacros.push_back(KeyAction{
      KeyTrigger{G5, 0, 0, ""},  // gKeyNumber=5
      {{KEY_LEFTCTRL, 1}, {KEY_V, 1}, {KEY_V, 0}, {KEY_LEFTCTRL, 0}},
      "Triggering G5 Ctrl+V macro",
      nullptr});

  // === APP-SPECIFIC MAPPINGS (override defaults) ===

  // --- TERMINAL ---
  appMacros_[AppType::TERMINAL] = defaultMacros;  // Start with defaults
  appMacros_[AppType::TERMINAL].push_back(KeyAction{
      KeyTrigger{G1, 0, 0, ""},  // gKeyNumber=1
      {{KEY_LEFTCTRL, 1}, {KEY_LEFTALT, 1}, {KEY_C, 1}, {KEY_C, 0},
       {KEY_LEFTALT, 0}, {KEY_LEFTCTRL, 0}},
      "Triggering G1 SIGINT macro (Ctrl+Alt+C) for Gnome Terminal",
      nullptr});
  appMacros_[AppType::TERMINAL].push_back(KeyAction{
      KeyTrigger{G_NONE, BTN_LEFT, KEY_LEFTCTRL, ""},  // keyCode=BTN_LEFT, modifiers=KEY_LEFTCTRL
      {{KEY_LEFTCTRL,0},{KEY_5, 1}, {KEY_5, 0}},
      "Triggering Terminal Ctrl+Left Click macro ( UNDO CONTROL AND 5)",
      nullptr});

  // --- CODE (VS Code) ---
  appMacros_[AppType::CODE] = defaultMacros;  // Start with defaults
  appMacros_[AppType::CODE].push_back(KeyAction{
      KeyTrigger{G2, 0, 0, ""},  // gKeyNumber=2
      {{KEY_END, 1}, {KEY_END, 0}},
      "Triggering G2 End macro for VS Code",
      nullptr});
  appMacros_[AppType::CODE].push_back(KeyAction{
      KeyTrigger{G6, 0, 0, ""},  // gKeyNumber=6
      {{KEY_LEFTCTRL, 1}, {KEY_C, 1}, {KEY_C, 0}, {KEY_LEFTCTRL, 0}},
      "Triggering G6 Ctrl+C macro for VS Code",
      nullptr});

  // --- CHROME ---
  appMacros_[AppType::CHROME] = defaultMacros;  // Start with defaults
  // Add Chrome ChatGPT Ctrl+V with callback
  appMacros_[AppType::CHROME].push_back(KeyAction{
      KeyTrigger{G_NONE, KEY_V, KEY_LEFTCTRL, "chatgpt.com"},  // Ctrl+V on chatgpt.com
      {},  // No key sequence (callback is used instead)
      "Triggering ChatGPT Ctrl+V macro (Speculative Parallel Focus)",
      [this]() { this->triggerChromeChatGPTMacro(); }});
  appMacros_[AppType::CHROME].push_back(KeyAction{
      KeyTrigger{G5, 0,  0, "chatgpt.com"},  // Ctrl+V on chatgpt.com
      {},  // No key sequence (callback is used instead)
      "Triggering ChatGPT Ctrl+V macro (Speculative Parallel Focus)",
      [this]() { this->triggerChromeChatGPTMacro(); }});

  // --- OTHER (default app type) ---
  appMacros_[AppType::OTHER] = defaultMacros;  // Start with defaults
}

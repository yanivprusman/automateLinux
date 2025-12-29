#include "InputMapper.h"
#include "Types.h"
#include "system.h" // Include system.h for executeBashCommand
#include <linux/input-event-codes.h>

void InputMapper::initializeAppMacros() {
// === DEFAULT MAPPINGS (apply to all apps unless overridden) ===
  std::vector<KeyAction> defaultMacros;
  // Mouse: Forward Button (press) -> Enter
  defaultMacros.push_back(
      KeyAction{KeyTrigger{{{BTN_FORWARD, KEY_PRESS, WITHHOLD_NO, KEY_REPEAT_BREAKS_NO}}},
                {{KEY_ENTER, 1}, {KEY_ENTER, 0}},
                "Triggering mouse forward button (press) → Enter",
                nullptr});
  // G6 (press) -> Ctrl+C
  defaultMacros.push_back(
      KeyAction{KeyTrigger{{{G6_VIRTUAL, KEY_PRESS, WITHHOLD_NO, KEY_REPEAT_BREAKS_NO}}},
                {{KEY_LEFTCTRL, 1}, {KEY_C, 1}, {KEY_C, 0}, {KEY_LEFTCTRL, 0}},
                "Triggering G6 (press) Ctrl+C macro",
                nullptr});

  // === APP-SPECIFIC MAPPINGS (override defaults) ===

  // Helper lambda to apply overrides
  auto applyOverrides = [&](const std::vector<KeyAction>& appSpecificMacros) {
      std::vector<KeyAction> finalMacros = defaultMacros;
      for (const auto& appMacro : appSpecificMacros) {
          bool overridden = false;
          for (auto& finalMacro : finalMacros) {
              if (finalMacro.trigger == appMacro.trigger) { // Uses the new operator==
                  finalMacro = appMacro; // Overwrite
                  overridden = true;
                  break;
              }
          }
          if (!overridden) {
              finalMacros.push_back(appMacro); // Add if not overriding
          }
      }
      return finalMacros;
  };


// --- TERMINAL ---
  std::vector<KeyAction> terminalSpecificMacros;
  terminalSpecificMacros.push_back(KeyAction{
      KeyTrigger{{{G1_VIRTUAL, KEY_PRESS, WITHHOLD_NO, KEY_REPEAT_BREAKS_NO}}},
      {{KEY_LEFTCTRL, 1},
       {KEY_LEFTALT, 1},
       {KEY_C, 1},
       {KEY_C, 0},
       {KEY_LEFTALT, 0},
       {KEY_LEFTCTRL, 0}},
      "Triggering G1 (press) SIGINT macro (Ctrl+Alt+C) for Gnome Terminal",
      nullptr});
  // // Ctrl(press, suppress) + LeftClick(press, suppress)
//   terminalSpecificMacros.push_back(
//       KeyAction{KeyTrigger{{{KEY_LEFTCTRL, KEY_PRESS, WITHHOLD_NO, KEY_REPEAT_BREAKS_NO}, {BTN_LEFT, KEY_PRESS, WITHHOLD_NO, KEY_REPEAT_BREAKS_NO}}},
//                 {{KEY_LEFTCTRL, KEY_RELEASE},{KEY_7, KEY_PRESS}, {KEY_7, KEY_RELEASE}}, // we must add the control up so that the 5 will print
//                 "Triggering Terminal Ctrl+Left Click macro",
//                 []() { 
//                     executeBashCommand("sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send \"hi\" \"2\"");
//                 }});
  terminalSpecificMacros.push_back(
      KeyAction{KeyTrigger{{{BTN_LEFT, KEY_PRESS, WITHHOLD_NO, KEY_REPEAT_BREAKS_NO}}},
                {{KEY_LEFTCTRL, KEY_RELEASE},{KEY_7, KEY_PRESS}, {KEY_7, KEY_RELEASE}}, // we must add the control up so that the 5 will print
                "Triggering Terminal Ctrl+Left Click macro",
                []() { 
                    executeBashCommand("sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send \"hi\" \"2\"");
                }});
//   terminalSpecificMacros.push_back(
//       KeyAction{KeyTrigger{{{KEY_A, KEY_PRESS, WITHHOLD_NO, KEY_REPEAT_BREAKS_NO}}},
//                 {},
//                 "Triggering Terminal Ctrl+Left Click macro",
//                 []() { 
//                     executeBashCommand("sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send \"hi\" \"2\"");
//                 }});
  appMacros_[AppType::TERMINAL] = applyOverrides(terminalSpecificMacros);

// --- CODE (VS Code) ---
  std::vector<KeyAction> codeSpecificMacros;
  codeSpecificMacros.push_back(
      KeyAction{KeyTrigger{{{G2_VIRTUAL, KEY_PRESS, WITHHOLD_NO, KEY_REPEAT_BREAKS_NO}}},
                {{KEY_END, KEY_PRESS}, {KEY_END, KEY_RELEASE}},
                "Triggering G2 (press) End macro for VS Code",
                nullptr});
  // Example override for CODE app: BTN_FORWARD (Mouse) -> KEY_A
  codeSpecificMacros.push_back(
      KeyAction{KeyTrigger{{{BTN_FORWARD, KEY_PRESS, WITHHOLD_NO, KEY_REPEAT_BREAKS_NO}}},
                {{KEY_A, 1}, {KEY_A, 0}},
                "Triggering mouse forward button (press) -> A in Code app (override default)",
                nullptr});
  appMacros_[AppType::CODE] = applyOverrides(codeSpecificMacros);

// --- CHROME ---
  std::vector<KeyAction> chromeSpecificMacros;
  // Ctrl(press, suppress) + V(press, suppress) with callback
  chromeSpecificMacros.push_back(
      KeyAction{KeyTrigger{{{KEY_LEFTCTRL, KEY_PRESS, WITHHOLD_NO, KEY_REPEAT_BREAKS_YES}, {KEY_V, KEY_PRESS, WITHHOLD_YES, KEY_REPEAT_BREAKS_NO}}},
                {}, // No key sequence (callback is used instead)
                "Triggering ChatGPT Ctrl+V macro (Focus + Paste)",
                [this]() { this->triggerChromeChatGPTMacro(); }});
  appMacros_[AppType::CHROME] = applyOverrides(chromeSpecificMacros);

  // --- OTHER (default app type) ---
  appMacros_[AppType::OTHER] = defaultMacros; // Start with defaults
}


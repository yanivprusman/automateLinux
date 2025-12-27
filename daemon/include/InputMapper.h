#ifndef INPUT_MAPPER_H
#define INPUT_MAPPER_H

#include "Types.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <vector>

// G-Key enumeration for type-safe G-key references
enum class GKey { G1 = 1, G2 = 2, G3 = 3, G4 = 4, G5 = 5, G6 = 6 };

// Virtual codes for G-keys to avoid collision with physical keys
#define G1_VIRTUAL 1001
#define G2_VIRTUAL 1002
#define G3_VIRTUAL 1003
#define G4_VIRTUAL 1004
#define G5_VIRTUAL 1005
#define G6_VIRTUAL 1006

// Represents the state of a key sequence combo during matching
struct ComboState {
  size_t nextKeyIndex =
      0; // Next key position to match (0 = start, size = complete)
  std::vector<std::pair<uint16_t, uint8_t>> suppressedKeys; // (keyCode, state) pairs held back for this combo
};

// Represents a keyboard/mouse trigger condition
// keyCodes is a sequence of (keyCode, state) pairs where state: 1=press, 0=release
struct KeyTrigger {
  std::vector<std::pair<uint16_t, uint8_t>> keyCodes; // Sequence of (key, state: 1=press, 0=release)
  bool suppressUnmatched = false;  // If true, withhold keys until combo matches or breaks
};

// Represents an action to execute when a trigger is matched
struct KeyAction {
  KeyTrigger trigger;
  std::vector<std::pair<uint16_t, int32_t>>
      keySequence;        // Key events to emit (empty if using callback)
  std::string logMessage; // Log message for the action
  std::function<void()> customHandler =
      nullptr; // Optional async handler (e.g., for Chrome ChatGPT)
};

class InputMapper {
public:
  InputMapper();
  ~InputMapper();

  bool start(const std::string &keyboardPath, const std::string &mousePath);
  void onFocusAck();
  void stop();
  void setContext(AppType appType, const std::string &url = "",
                  const std::string &title = "");
  bool isRunning() const { return running_; }

private:
  void loop();
  bool setupDevices();
  bool setupUinput();
  void processEvent(struct input_event &ev, bool isKeyboard, bool skipMacros);
  void emit(uint16_t type, uint16_t code, int32_t value);
  void emitSequence(const std::vector<std::pair<uint16_t, int32_t>> &sequence);
  std::optional<GKey> detectGKey(const struct input_event &ev);
  void executeKeyAction(const KeyAction &action);
  void initializeAppMacros();
  void triggerChromeChatGPTMacro();

  std::string keyboardPath_;
  std::string mousePath_;
  int keyboardFd_ = -1;
  int mouseFd_ = -1;
  struct libevdev *keyboardDev_ = nullptr;
  struct libevdev *mouseDev_ = nullptr;
  struct libevdev_uinput *uinputDev_ = nullptr;

  std::thread thread_;
  std::atomic<bool> running_{false};
  bool ctrlDown_ = false; // Track LeftCtrl state for macros

  // Thread safety and async flow
  std::mutex uinputMutex_;
  std::atomic<bool> withholdingV_{false};
  std::chrono::steady_clock::time_point lastWithholdingStart_;

  // --- State Machine for evsieve logic ---

  // G-key sequence detection (corresponds to GToggle in evsieve script)
  // Sequence: Ctrl(down) -> Shift(down) -> Ctrl(down) -> Shift(down) ->
  // Key(number)
  int gToggleState_ = 1; // 1-5

  // Window/App detection context
  AppType activeApp_ = AppType::OTHER;
  std::string activeUrl_;
  std::string activeTitle_;
  std::mutex contextMutex_;

  // App-specific macro mappings
  std::map<AppType, std::vector<KeyAction>> appMacros_;

  // Combo sequence tracking (per-app)
  std::map<AppType, std::map<size_t, ComboState>>
      comboProgress_; // appType → (comboIndex → progress)
  std::map<uint16_t, std::set<size_t>>
      keySuppressedBy_; // keyCode → {combo indices suppressing it}

  // Key event queue for delayed emission when combos break (~1ms delay)
  struct PendingKeyEvent {
    uint16_t keyCode;
    uint8_t state;  // 1=press, 0=release
    std::chrono::steady_clock::time_point emitTime;
  };
  std::vector<PendingKeyEvent> pendingKeyEvents_;
  std::mutex pendingKeyEventsMutex_;
};

#endif // INPUT_MAPPER_H

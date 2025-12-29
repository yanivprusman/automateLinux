#ifndef INPUT_MAPPER_H
#define INPUT_MAPPER_H

#include "Types.h"
#include "using.h"
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

// Structure to define a granular input log filter
struct InputLogFilter {
  std::optional<uint16_t> type;
  std::optional<uint16_t> code;
  std::optional<int32_t> value;
  std::optional<std::string> devicePathRegex;
  std::optional<bool> isKeyboard;
  bool actionShow; // true to show, false to hide

  // Comparison operator for use in std::set or std::sort
  // More specific filters should come before less specific ones.
  bool operator<(const InputLogFilter& other) const {
      // Prioritize by number of defined fields (more defined = more specific)
      int this_specificity = (type.has_value() ? 1 : 0) +
                             (code.has_value() ? 1 : 0) +
                             (value.has_value() ? 1 : 0) +
                             (devicePathRegex.has_value() ? 1 : 0) +
                             (isKeyboard.has_value() ? 1 : 0);
      int other_specificity = (other.type.has_value() ? 1 : 0) +
                              (other.code.has_value() ? 1 : 0) +
                              (other.value.has_value() ? 1 : 0) +
                              (other.devicePathRegex.has_value() ? 1 : 0) +
                              (other.isKeyboard.has_value() ? 1 : 0);

      if (this_specificity != other_specificity) {
          return this_specificity > other_specificity; // Higher specificity comes first
      }

      // Fallback to lexicographical comparison for consistent ordering
      if (type != other.type) return type < other.type;
      if (code != other.code) return code < other.code;
      if (value != other.value) return value < other.value;
      if (devicePathRegex != other.devicePathRegex) return devicePathRegex < other.devicePathRegex;
      if (isKeyboard != other.isKeyboard) return isKeyboard < other.isKeyboard;
      return actionShow < other.actionShow;
  }
};

// Represents the state of a key sequence combo during matching
struct ComboState {
  size_t nextKeyIndex =
      0; // Next key position to match (0 = start, size = complete)
  std::vector<std::pair<uint16_t, uint8_t>>
      suppressedKeys; // (keyCode, state) pairs held back for this combo
};

// Represents a keyboard/mouse trigger condition
// keyCodes is a sequence of (keyCode, state, suppress) tuples where:
// - state: 1=press, 0=release
// - suppress: true=withhold this key until combo resolves
struct KeyTrigger {
  std::vector<std::tuple<uint16_t, uint8_t, bool, bool>>
      keyCodes; // Sequence of (code, state: 1=press/0=release, suppress:
                // true/false, ignoreRepeat: true/false)
  bool hasSuppressedKeys = false; // Cached check for efficiency
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

  void loadPersistence();

  bool start(const std::string &keyboardPath, const std::string &mousePath);
  void stop();
  void setContext(AppType appType, const std::string &url = "",
                  const std::string &title = "");
  void flushAndResetState();
  void onFocusAck();
  bool isRunning() const { return running_; }
  void setPendingGrab(bool value); // NEW PUBLIC SETTER

  json getMacrosJson();
  json getEventFiltersJson();
  json getActiveContextJson();
  void setMacrosFromJson(const json &j);
  void setEventFilters(const json &j);
  void emit(uint16_t type, uint16_t code, int32_t value);
  void sync();

private:
  void setMacrosFromJsonInternal(const json &j);
  void setEventFiltersInternal(const json &j);
  void grabDevices();   // New: performs the libevdev_grab
  void ungrabDevices(); // New: performs the libevdev_ungrab

private:
  void loop();
  bool setupDevices();
  bool setupUinput();
  void processEvent(struct input_event &ev, bool isKeyboard, bool skipMacros, const std::string& devicePath);
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
  std::set<uint16_t> pressedKeys_; // To track currently pressed keys
  std::atomic<bool> pendingGrab_{false}; // True if grab is desired but waiting for keys to be released
  std::atomic<bool> monitoringMode_{false}; // True if devices are open but not grabbed
  bool ctrlDown_ = false; // Track LeftCtrl state for macros

  // Thread safety and async flow
  std::mutex uinputMutex_;

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
  std::mutex macrosMutex_;

  // Event filtering state (granular logging)
  std::set<uint16_t> filteredKeyCodes_;
  std::mutex filtersMutex_;

  // Granular Input Log Filters
  std::vector<InputLogFilter> inputLogFilters_;
  std::mutex inputLogFiltersMutex_;
  void setInputLogFiltersInternal(const json &j);
  json getInputLogFiltersJson();

  // Combo sequence tracking (per-app)
  std::map<AppType, std::map<size_t, ComboState>>
      comboProgress_; // appType → (comboIndex → progress)
  std::map<uint16_t, std::set<size_t>>
      keySuppressedBy_; // keyCode → {combo indices suppressing it}

  // Key event queue for delayed emission when combos break
  struct PendingEvent {
    uint16_t type;
    uint16_t code;
    int32_t value;
  };
  std::vector<PendingEvent> pendingEvents_;
  std::mutex pendingEventsMutex_;

  // Focus synchronization
  std::condition_variable focusAckCv_;
  std::mutex focusAckMutex_;
  std::atomic<bool> focusAckReceived_{false};
};

#endif // INPUT_MAPPER_H

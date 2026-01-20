#ifndef INPUT_MAPPER_H
#define INPUT_MAPPER_H

enum class PipelineResult {
  CONTINUE, // Pass to next stage
  CONSUMED, // Stop processing, assume handled
  DROP      // Stop processing, ignore event
};

#include "Types.h"
#include "common.h"
#include <algorithm>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iterator>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;

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
  std::vector<std::tuple<uint16_t, uint16_t, int32_t>>
      suppressedEvents;                                // (type, code, value)
  std::chrono::steady_clock::time_point lastMatchTime; // For timeout support
};

// Represents a keyboard/mouse trigger condition
// events is a sequence of (type, code, value, suppress, ignoreRepeat) tuples
struct KeyTrigger {
  std::vector<std::tuple<uint16_t, uint16_t, int32_t, bool, bool>>
      events; // Sequence of (type, code, value, suppress, ignoreRepeat)
  bool hasSuppressedKeys = false; // Cached check for efficiency
  bool noiseBreaks = true; // If false, unrelated keys won't reset progress

  // Operator== for KeyTrigger
  bool operator==(const KeyTrigger &other) const {
    return events == other.events &&
           hasSuppressedKeys == other.hasSuppressedKeys &&
           noiseBreaks == other.noiseBreaks;
  }
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
  void setNumLockState(bool active); // NEW PUBLIC SETTER for NumLock state

  json getMacrosJson();
  json getEventFiltersJson();
  json getActiveContextJson();
  void setMacrosFromJson(const json &j);
  void setEventFilters(const json &j);
  void emit(uint16_t type, uint16_t code, int32_t value);
  void emitNoSync(uint16_t type, uint16_t code, int32_t value);
  void sync();

private:
  void setMacrosFromJsonInternal(const json &j);
  void setEventFiltersInternal(const json &j);
public:
  void grabDevices();   // New: performs the libevdev_grab
  void ungrabDevices(); // New: performs the libevdev_ungrab

private:
  void loop();
  bool setupDevices();
  bool setupUinput();
  void processEvent(struct input_event &ev, bool isKeyboard, bool skipMacros,
                    const std::string &devicePath);
  void emitSequence(const std::vector<std::pair<uint16_t, int32_t>> &sequence);
  std::optional<GKey> detectGKey(const struct input_event &ev);
  void executeKeyAction(const KeyAction &action);
  bool isKeyboardOnlyMacro(const KeyAction &action) const;
  void releaseAllPressedKeys();
  std::string formatEvent(const struct input_event &ev,
                          const std::string &devicePath) const;
  void initializeAppMacros();
  void triggerChromeChatGPTMacro();
  void triggerPublicTransportationMacro();

  std::string keyboardPath_;
  std::string mousePath_;
  int keyboardFd_ = -1;
  int mouseFd_ = -1;
  struct libevdev *keyboardDev_ = nullptr;
  struct libevdev *mouseDev_ = nullptr;
  struct libevdev_uinput *uinputDev_ = nullptr;

  std::thread thread_;
  std::atomic<bool> running_{false};
  std::set<uint16_t> pressedKeys_;
  std::mutex pressedKeysMutex_;
  std::atomic<bool> monitoringMode_{
      false};                  // True if devices are open but not grabbed
  bool ctrlDown_ = false;      // Track LeftCtrl state for macros
  bool numLockActive_ = false; // NEW: Track NumLock state to disable macros

  // Thread safety and async flow
  std::mutex uinputMutex_;

  // Represents a key being held back by a macro
  struct WithheldKey {
    uint16_t type;
    uint16_t code;
    int32_t value;
    size_t owningComboIdx;
  };

  // Tracks which macros are withholding which physical keys
  struct SuppressionRegistry {
    std::map<uint16_t, std::vector<WithheldKey>> withheld;
    std::mutex mutex;

    void add(uint16_t type, uint16_t code, int32_t value, size_t comboIdx) {
      std::lock_guard<std::mutex> lock(mutex);
      withheld[code].push_back({type, code, value, comboIdx});
    }

    // Returns all withheld events for a combo that just broke/finished
    std::vector<WithheldKey> claim(size_t comboIdx) {
      std::lock_guard<std::mutex> lock(mutex);
      std::vector<WithheldKey> claimed;
      for (auto &pair : withheld) {
        auto &vec = pair.second;
        auto it = std::remove_if(vec.begin(), vec.end(),
                                 [comboIdx](const WithheldKey &wk) {
                                   return wk.owningComboIdx == comboIdx;
                                 });
        std::copy(it, vec.end(), std::back_inserter(claimed));
        vec.erase(it, vec.end());
      }
      return claimed;
    }

    bool isBlocked(uint16_t code) {
      std::lock_guard<std::mutex> lock(mutex);
      return !withheld[code].empty();
    }
  };

  SuppressionRegistry suppressionRegistry_;

  // G-key sequence detection (corresponds to GToggle in evsieve script)
  // Sequence: Ctrl(down) -> Shift(down) -> Ctrl(down) -> Shift(down) ->
  // Key(number)
  int gToggleState_ = 1; // 1-5

  // Emergency ungrab: ScrollLock triple-press within 1 second
  int emergencyScrollCount_ = 0;
  std::chrono::steady_clock::time_point emergencyFirstPress_;

  // Window/App detection context
  AppType activeApp_ = AppType::OTHER;
  std::string activeUrl_;
  std::string activeTitle_;
  std::mutex contextMutex_;

  // App-specific macro mappings
  std::map<AppType, std::vector<KeyAction>> appMacros_;
  std::mutex macrosMutex_;

  // Event filtering state (granular logging)
  std::vector<std::string> logFilterPatterns_;
  std::mutex filtersMutex_;

  // Combo sequence tracking (per-app)
  std::map<AppType, std::map<size_t, ComboState>>
      comboProgress_; // appType → (comboIndex → progress)
  std::map<uint16_t, std::set<size_t>>
      keySuppressedBy_; // keyCode → {combo indices suppressing it}

  // Internal Pipeline Stages
  PipelineResult stageContext(struct input_event &ev,
                              const std::string &devicePath);
  PipelineResult stageGKey(struct input_event &ev);
  PipelineResult stageMacros(struct input_event &ev, bool skipMacros);
  void emitFinal(const struct input_event &ev,
                 const std::string & /*devicePath*/);
  bool shouldLog(const std::string &eventStr, uint32_t category);

  // Focus synchronization
  std::condition_variable focusAckCv_;
  std::mutex focusAckMutex_;
  std::atomic<bool> focusAckReceived_{false};
};

#endif // INPUT_MAPPER_H

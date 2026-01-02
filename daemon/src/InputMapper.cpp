#include "InputMapper.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "Globals.h"
#include "Utils.h"
#include "using.h"
#include <algorithm>
#include <atomic>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <linux/input-event-codes.h>
#include <mutex>
#include <poll.h>
#include <regex> // For devicePathRegex matching
#include <thread>
#include <unistd.h>
#include <vector>

using namespace std;

InputMapper::InputMapper() { initializeAppMacros(); }

InputMapper::~InputMapper() { stop(); }

void InputMapper::loadPersistence() {
  // Load Macros from DB
  string savedMacros = ConfigTable::getConfig("custom_macros");
  if (!savedMacros.empty()) {
    try {
      json j = json::parse(savedMacros);
      {
        std::lock_guard<std::mutex> lock(macrosMutex_);
        setMacrosFromJsonInternal(j);
      }
      logToFile("Loaded custom macros from DB", LOG_CORE);
    } catch (...) {
      logToFile("Failed to parse saved macros from DB", LOG_CORE);
    }
  }

  // Load Filters from DB
  string savedFilters = ConfigTable::getConfig("custom_event_filters");
  if (!savedFilters.empty()) {
    try {
      json j = json::parse(savedFilters);
      {
        std::lock_guard<std::mutex> lock(filtersMutex_);
        setEventFiltersInternal(j);
      }
      logToFile("Loaded custom event filters from DB", LOG_CORE);
    } catch (...) {
      logToFile("Failed to parse saved filters from DB", LOG_CORE);
    }
  }
}

bool InputMapper::start(const std::string &keyboardPath,
                        const std::string &mousePath) {
  keyboardPath_ = keyboardPath;
  mousePath_ = mousePath;

  if (!setupDevices()) {
    stop();
    return false;
  }
  if (!setupUinput()) {
    stop();
    return false;
  }

  running_ = true;
  thread_ = std::thread(&InputMapper::loop, this);
  return true;
}

void InputMapper::stop() {
  logToFile("InputMapper: Stopping...", LOG_CORE);
  running_ = false;
  if (thread_.joinable()) {
    logToFile("InputMapper: Joining thread...", LOG_CORE);
    thread_.join();
    logToFile("InputMapper: Thread joined", LOG_CORE);
  }

  // If devices were grabbed, ungrab them first
  if (!monitoringMode_) {
    ungrabDevices();
  }

  if (uinputDev_) {
    libevdev_uinput_destroy(uinputDev_);
    uinputDev_ = nullptr;
  }
  if (keyboardDev_) {
    libevdev_grab(keyboardDev_, LIBEVDEV_UNGRAB);
    libevdev_free(keyboardDev_);
    keyboardDev_ = nullptr;
  }
  if (mouseDev_) {
    libevdev_grab(mouseDev_, LIBEVDEV_UNGRAB);
    libevdev_free(mouseDev_);
    mouseDev_ = nullptr;
  }
  if (keyboardFd_ >= 0) {
    close(keyboardFd_);
    keyboardFd_ = -1;
  }
  if (mouseFd_ >= 0) {
    close(mouseFd_);
    mouseFd_ = -1;
  }
  logToFile("InputMapper: Stopped", LOG_CORE);
}

bool InputMapper::setupDevices() {
  keyboardFd_ = open(keyboardPath_.c_str(), O_RDONLY | O_NONBLOCK);
  if (keyboardFd_ < 0) {
    logToFile("Failed to open keyboard device: " + keyboardPath_, LOG_CORE);
    return false;
  }

  if (libevdev_new_from_fd(keyboardFd_, &keyboardDev_) < 0) {
    logToFile("Failed to initialize libevdev for keyboard", LOG_CORE);
    return false;
  }

  // Query initial state of all keys and populate pressedKeys_
  for (int code = 0; code < KEY_CNT; ++code) {
    if (libevdev_has_event_code(keyboardDev_, EV_KEY, code) &&
        libevdev_get_event_value(keyboardDev_, EV_KEY, code) == 1) {
      pressedKeys_.insert(code);
      logToFile("DEBUG_KEYS: Initial pressed key: " + std::to_string(code),
                LOG_MACROS);
    }
  }
  logToFile("DEBUG_KEYS: Initial pressed keys count: " +
                std::to_string(pressedKeys_.size()),
            LOG_MACROS);

  if (!mousePath_.empty()) {
    mouseFd_ = open(mousePath_.c_str(), O_RDONLY | O_NONBLOCK);
    if (mouseFd_ >= 0) {
      if (libevdev_new_from_fd(mouseFd_, &mouseDev_) < 0) {
        logToFile("Failed to initialize libevdev for mouse", LOG_CORE);
      } else {
        // Scan for pressed mouse buttons (e.g. holding click)
        for (int code = BTN_MOUSE; code < BTN_JOYSTICK; ++code) {
          if (libevdev_has_event_code(mouseDev_, EV_KEY, code) &&
              libevdev_get_event_value(mouseDev_, EV_KEY, code) == 1) {
            pressedKeys_.insert(code);
            logToFile("DEBUG_KEYS: Initial pressed mouse btn: " +
                          std::to_string(code),
                      LOG_MACROS);
          }
        }
      }
    }
  }

  monitoringMode_ = true; // Devices are open but not grabbed
  return true;
}

bool InputMapper::setupUinput() {
  struct libevdev *uinput_template = libevdev_new();
  libevdev_set_name(uinput_template, "AutomateLinux Virtual Device");

  // Set ID metadata to look like a real USB device.
  // This helps desktop environments (GNOME/Mutter) recognize it as a valid
  // input source.
  libevdev_set_id_bustype(uinput_template, BUS_USB);
  libevdev_set_id_vendor(uinput_template, 0x1b1c);  // Corsair
  libevdev_set_id_product(uinput_template, 0x1bc5); // K100
  libevdev_set_id_version(uinput_template, 0x0111);

  // Helper to enable all bits from a source device
  auto enable_bits = [&](struct libevdev *src) {
    for (int type = 0; type < EV_MAX; ++type) {
      if (libevdev_has_event_type(src, type)) {
        libevdev_enable_event_type(uinput_template, type);
        int max_code = libevdev_event_type_get_max(type);
        if (max_code < 0)
          continue;
        for (int code = 0; code <= max_code; ++code) {
          if (libevdev_has_event_code(src, type, code)) {
            libevdev_enable_event_code(uinput_template, type, code,
                                       libevdev_get_abs_info(src, code));
            // Force log to bypass shouldLog filter for debugging
            extern void forceLog(const std::string &message);
            std::string debugMsg =
                "Enabled event code: type=" + std::to_string(type) +
                ", code=" + std::to_string(code);
            forceLog(debugMsg);
          }
        }
      }
    }
  };

  if (keyboardDev_)
    enable_bits(keyboardDev_);
  if (mouseDev_)
    enable_bits(mouseDev_);

  // Robustness: enable ALL possible keys
  for (int i = 0; i < KEY_MAX; ++i) {
    libevdev_enable_event_code(uinput_template, EV_KEY, i, NULL);
  }

  // ensure some common types are enabled even if not in physical devices
  // (optional but good)
  libevdev_enable_event_type(uinput_template, EV_KEY);
  libevdev_enable_event_type(uinput_template, EV_SYN);

  int rc = libevdev_uinput_create_from_device(
      uinput_template, LIBEVDEV_UINPUT_OPEN_MANAGED, &uinputDev_);
  libevdev_free(uinput_template);

  if (rc < 0) {
    logToFile("Failed to create uinput device: " + std::string(strerror(-rc)),
              LOG_CORE);
    return false;
  }

  return true;
}

json InputMapper::getMacrosJson() {
  std::lock_guard<std::mutex> lock(macrosMutex_);
  json j = json::object();
  for (const auto &pair : appMacros_) {
    string appName = appTypeToString(pair.first);
    json appMacros = json::array();
    for (const auto &action : pair.second) {
      json a;
      a["logMessage"] = action.logMessage;
      json trigger;
      json triggerKeys = json::array();
      for (const auto &eventTuple : action.trigger.events) {
        json e;
        e["type"] = std::get<0>(eventTuple);
        e["code"] = std::get<1>(eventTuple);
        e["state"] = std::get<2>(eventTuple); // 'state' is used for JSON value
        e["suppress"] = std::get<3>(eventTuple);
        e["ignoreRepeat"] = std::get<4>(eventTuple);
        triggerKeys.push_back(e);
      }
      trigger["keys"] = triggerKeys;
      a["trigger"] = trigger;

      json seq = json::array();
      for (const auto &sk : action.keySequence) {
        seq.push_back({sk.first, sk.second});
      }
      a["sequence"] = seq;
      a["hasHandler"] = (action.customHandler != nullptr);
      appMacros.push_back(a);
    }
    j[appName] = appMacros;
  }
  return j;
}

json InputMapper::getActiveContextJson() {
  std::lock_guard<std::mutex> lock(contextMutex_);
  json j;
  j["activeApp"] = appTypeToString(activeApp_);
  j["activeUrl"] = activeUrl_;
  j["activeTitle"] = activeTitle_;
  j["numLockActive"] = (bool)numLockActive_;
  return j;
}

json InputMapper::getEventFiltersJson() {
  std::lock_guard<std::mutex> lock(filtersMutex_);
  json j = json::array();
  for (const string &pattern : logFilterPatterns_) {
    j.push_back(pattern);
  }
  return j;
}

void InputMapper::setMacrosFromJsonInternal(const json &j) {
  // Ensure defaults are present if map is empty (though constructor calls init)
  if (appMacros_.empty()) {
    initializeAppMacros();
  }
  // Clear combo progress to prevent stale states
  comboProgress_.clear();

  // We DO NOT clear appMacros_ here because we want to keep defaults.
  // Instead, the JSON loading acts as an overlay/addition.
  // If the user wants to truly 'reset', they must restart daemon or we need a
  // separate reset command.
  for (auto it = j.begin(); it != j.end(); ++it) {
    AppType app = stringToAppType(it.key());
    vector<KeyAction> macros;
    for (const auto &ma : it.value()) {
      KeyAction action;
      action.logMessage = ma.value("message", "");

      KeyTrigger trigger;
      if (ma.contains("trigger")) {
        trigger.noiseBreaks = ma["trigger"].value("noiseBreaks", true);
        if (ma["trigger"].contains("keys")) {
          for (const auto &tk : ma["trigger"]["keys"]) {
            if (tk.is_object()) {
              uint16_t type = tk.value("type", (uint16_t)EV_KEY);
              uint16_t code = tk.value("code", (uint16_t)0);
              int32_t value = tk.value("state", 0); // Handle 'state' as value
              bool suppress = tk.value("suppress", false);
              bool ignoreRepeat = tk.value("ignoreRepeat", false);
              trigger.events.push_back(
                  {type, code, value, suppress, ignoreRepeat});
              if (suppress)
                trigger.hasSuppressedKeys = true;
            } else if (tk.is_array() && tk.size() >= 2) {
              // Backward compatibility for array-style keys [code, state,
              // suppress, ignoreRepeat]
              uint16_t type = EV_KEY;
              uint16_t code = tk[0].get<uint16_t>();
              int32_t value = tk[1].get<int32_t>();
              bool suppress = tk.size() >= 3 ? tk[2].get<bool>() : false;
              bool ignoreRepeat = tk.size() >= 4 ? tk[3].get<bool>() : false;
              trigger.events.push_back(
                  {type, code, value, suppress, ignoreRepeat});
              if (suppress)
                trigger.hasSuppressedKeys = true;
            }
          }
        }
        action.trigger = trigger;

        if (ma.contains("sequence")) {
          for (const auto &seq : ma["sequence"]) {
            if (seq.is_array() && seq.size() >= 2) {
              action.keySequence.push_back(
                  {seq[0].get<uint16_t>(), seq[1].get<int32_t>()});
            }
          }
        }

        // Re-bind internal handlers by message or other logic if needed
        if (ma.value("hasHandler", false)) {
          if (action.logMessage.find("ChatGPT") != string::npos) {
            action.customHandler = [this]() {
              this->triggerChromeChatGPTMacro();
            };
          }
        }

        macros.push_back(action);
      }
      appMacros_[app] = macros;
    }
  }
}

void InputMapper::setMacrosFromJson(const json &j) {
  {
    std::lock_guard<std::mutex> lock(macrosMutex_);
    setMacrosFromJsonInternal(j);
  }
  ConfigTable::setConfig("custom_macros", j.dump());
  logToFile("Macros updated dynamically and saved to DB", LOG_CORE);
}

void InputMapper::setEventFiltersInternal(const json &j) {
  logFilterPatterns_.clear();
  if (j.is_array()) {
    for (const auto &item : j) {
      if (item.is_string()) {
        logFilterPatterns_.push_back(item.get<string>());
      }
    }
  }
}

void InputMapper::setEventFilters(const json &j) {
  {
    std::lock_guard<std::mutex> lock(filtersMutex_);
    setEventFiltersInternal(j);
  }
  ConfigTable::setConfig("custom_event_filters", j.dump());
  logToFile("Event filters updated dynamically and saved to DB (count: " +
                std::to_string(logFilterPatterns_.size()) + ")",
            LOG_CORE);
}

void InputMapper::loop() {
  struct pollfd fds[2];
  fds[0].fd = keyboardFd_;
  fds[0].events = POLLIN;
  fds[1].fd = mouseFd_;
  fds[1].events = POLLIN;

  int nfds = (mouseFd_ >= 0) ? 2 : 1;

  logToFile("InputMapper loop starting...", LOG_CORE);
  while (running_) {
    int rc = poll(fds, nfds, 100); // 100ms timeout
    if (rc < 0)
      break;
    if (rc == 0)
      continue;

    if (fds[0].revents & POLLIN) {
      struct input_event ev;
      int rc_event;
      while ((rc_event = libevdev_next_event(keyboardDev_,
                                             LIBEVDEV_READ_FLAG_NORMAL, &ev)) ==
                 LIBEVDEV_READ_STATUS_SUCCESS ||
             rc_event == LIBEVDEV_READ_STATUS_SYNC) {
        if (rc_event == LIBEVDEV_READ_STATUS_SYNC) {
          logToFile("[InputMapper] Keyboard Sync Status!", LOG_CORE);
          flushAndResetState();
          while (libevdev_next_event(keyboardDev_, LIBEVDEV_READ_FLAG_SYNC,
                                     &ev) == LIBEVDEV_READ_STATUS_SYNC) {
            processEvent(ev, true, true,
                         keyboardPath_); // true for sync (skip macros)
          }
          continue;
        }
        processEvent(ev, true, false, keyboardPath_);
      }
    }

    if (mouseFd_ >= 0 && (fds[1].revents & POLLIN)) {
      struct input_event ev;
      int rc_event;
      while ((rc_event = libevdev_next_event(mouseDev_,
                                             LIBEVDEV_READ_FLAG_NORMAL, &ev)) ==
                 LIBEVDEV_READ_STATUS_SUCCESS ||
             rc_event == LIBEVDEV_READ_STATUS_SYNC) {
        if (rc_event == LIBEVDEV_READ_STATUS_SYNC) {
          logToFile("[InputMapper] Mouse Sync Status!", LOG_CORE);
          flushAndResetState();
          while (libevdev_next_event(mouseDev_, LIBEVDEV_READ_FLAG_SYNC, &ev) ==
                 LIBEVDEV_READ_STATUS_SYNC) {
            processEvent(ev, false, true, mousePath_); // true for sync
          }
          continue;
        }
        processEvent(ev, false, false, mousePath_);
      }
    }
  }
}

void InputMapper::emit(uint16_t type, uint16_t code, int32_t value) {
  if (!uinputDev_)
    return;
  std::lock_guard<std::mutex> lock(uinputMutex_);
  libevdev_uinput_write_event(uinputDev_, type, code, value);
  libevdev_uinput_write_event(uinputDev_, EV_SYN, SYN_REPORT, 0);
}

void InputMapper::sync() {
  if (!uinputDev_)
    return;
  std::lock_guard<std::mutex> lock(uinputMutex_);
  libevdev_uinput_write_event(uinputDev_, EV_SYN, SYN_REPORT, 0);
}


void InputMapper::emitSequence(
    const std::vector<std::pair<uint16_t, int32_t>> &sequence) {
  if (!uinputDev_)
    return;
  std::lock_guard<std::mutex> lock(uinputMutex_);
  for (const auto &p : sequence) {
    libevdev_uinput_write_event(uinputDev_, EV_KEY, p.first, p.second);
    libevdev_uinput_write_event(uinputDev_, EV_SYN, SYN_REPORT, 0);
  }
}

void InputMapper::grabDevices() {
  extern void forceLog(const std::string &message);

  {
    std::lock_guard<std::mutex> lock(pressedKeysMutex_);
    if (!pressedKeys_.empty()) {
      std::string keys;
      for (uint16_t code : pressedKeys_) {
        const char *name = libevdev_event_code_get_name(EV_KEY, code);
        keys += (name ? name : std::to_string(code)) + " ";
      }
      forceLog("InputMapper: Not grabbing, keys are pressed: " + keys);
      return;
    }
  }

  forceLog("InputMapper: Attempting GRAB...");
  if (keyboardDev_) {
    if (libevdev_grab(keyboardDev_, LIBEVDEV_GRAB) < 0) {
      forceLog("Failed to grab keyboard");
    } else {
      forceLog("InputMapper: Keyboard grabbed successfully");
    }
  }
  if (mouseDev_) {
    if (libevdev_grab(mouseDev_, LIBEVDEV_GRAB) < 0) {
      forceLog("Failed to grab mouse");
    } else {
      forceLog("InputMapper: Mouse grabbed successfully");
    }
  }
  monitoringMode_ = false; // We are no longer just monitoring
  forceLog("InputMapper: GRAB complete. monitoringMode_=false");
}

void InputMapper::ungrabDevices() {
  extern void forceLog(const std::string &message);
  if (keyboardDev_) {
    libevdev_grab(keyboardDev_, LIBEVDEV_UNGRAB);
    forceLog("InputMapper: Keyboard ungrabbed");
  }
  if (mouseDev_) {
    libevdev_grab(mouseDev_, LIBEVDEV_UNGRAB);
    forceLog("InputMapper: Mouse ungrabbed");
  }
  monitoringMode_ =
      true; // We are back to monitoring (if devices are still open)
}

void InputMapper::setContext(AppType appType, const std::string &url,
                             const std::string &title) {
  {
    std::lock_guard<std::mutex> lock(contextMutex_);
    if (activeApp_ == appType && activeUrl_ == url && activeTitle_ == title) {
      return; // No change
    }
    activeApp_ = appType;
    activeUrl_ = url;
    activeTitle_ = title;
  }

  logToFile("Context updated: App=[" + appTypeToString(appType) + "] URL=[" +
                url + "] Title=[" + title + "]",
            LOG_WINDOW);

  // Release keys and clear pending events on window change to prevent stuck
  // keys/macros
  releaseAllPressedKeys();
}

void InputMapper::processEvent(struct input_event &ev, bool isKeyboard,
                               bool isMouse, const std::string &devicePath) {
  (void)isKeyboard;
  // Stage 1: Context & Tracking (Log, NumLock, Ctrl, pressedKeys)
  // We do this BEFORE monitoringMode_ check so state is tracked even if not
  // grabbing.
  if (stageContext(ev, devicePath) == PipelineResult::DROP)
    return;

  // If we are in monitoring mode (devices open but not grabbed),
  // we do NOT emit events to uinput to avoid double-input.
  if (monitoringMode_) {
    // Only log if filter allows
    string formattedEvent = formatEvent(ev, devicePath);
    if (!formattedEvent.empty() && shouldLog(formattedEvent, LOG_INPUT_DEBUG)) {
      logToFile("SKIPPED (monitoring): Type=" + std::to_string(ev.type) +
                    " Code=" + std::to_string(ev.code),
                LOG_INPUT_DEBUG);
    }
    return;
  }

  // --- PIPELINE START ---

  // Stage 2: G-Key Interaction
  if (!isMouse && stageGKey(ev) == PipelineResult::CONSUMED)
    return;

  // Stage 3: Macro Matching & Suppression
  if (stageMacros(ev, isMouse) == PipelineResult::DROP)
    return;

  // Stage 4: Final Emission
  emitFinal(ev, devicePath);
}

PipelineResult InputMapper::stageContext(struct input_event &ev,
                                         const std::string &devicePath) {
  // Global logging logic
  string formattedEvent = formatEvent(ev, devicePath);
  if (!formattedEvent.empty()) {
    if (shouldLog(formattedEvent, LOG_INPUT))
      logToFile(formattedEvent, LOG_INPUT);
  }

  // Debug: Log all Ctrl+V combinations to see current context
  if (ev.type == EV_KEY && ev.code == KEY_V && ctrlDown_ && ev.value == 1) {
    AppType tmpApp;
    std::string tmpUrl;
    {
      std::lock_guard<std::mutex> lock(contextMutex_);
      tmpApp = activeApp_;
      tmpUrl = activeUrl_;
    }
    logToFile("Ctrl+V detected. State: App=[" + appTypeToString(tmpApp) +
                  "] URL=[" + tmpUrl + "]",
              LOG_MACROS);
  }

  if (ev.type == EV_KEY) {
    if (ev.value == 1) {
      std::lock_guard<std::mutex> lock(pressedKeysMutex_);
      pressedKeys_.insert(ev.code);
      logToFile("DEBUG_KEYS: Key " + std::to_string(ev.code) +
                    " pressed. Total: " + std::to_string(pressedKeys_.size()),
                LOG_MACROS);
    } else if (ev.value == 0) {
      std::lock_guard<std::mutex> lock(pressedKeysMutex_);
      pressedKeys_.erase(ev.code);
      logToFile("DEBUG_KEYS: Key " + std::to_string(ev.code) +
                    " released. Total: " + std::to_string(pressedKeys_.size()),
                LOG_MACROS);
    }

    if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL) {
      ctrlDown_ = (ev.value != 0);
    }

    if (ev.code == KEY_NUMLOCK && ev.value == 1) {
      setNumLockState(!numLockActive_);
    }
  }
  return PipelineResult::CONTINUE;
}

PipelineResult InputMapper::stageGKey(struct input_event &ev) {
  auto gKey = detectGKey(ev);
  if (gKey) {
    logToFile("VIRTUAL G-KEY DETECTED: G" +
                  std::to_string(static_cast<int>(*gKey)),
              LOG_AUTOMATION);
    struct input_event virtualEv = ev;
    virtualEv.code = 1000 + static_cast<uint16_t>(*gKey);
    processEvent(virtualEv, true, false,
                 keyboardPath_); // Recursive call for virtual event
    return PipelineResult::CONSUMED;
  }
  return PipelineResult::CONTINUE;
}

PipelineResult InputMapper::stageMacros(struct input_event &ev,
                                        bool skipMacros) {
  if (skipMacros) {
    if (ev.type == EV_KEY && numLockActive_) {
      logToFile("Macros DISABLED (NumLock ON)", LOG_MACROS);
    }
    return PipelineResult::CONTINUE;
  }

  AppType currentApp;
  {
    std::lock_guard<std::mutex> lock(contextMutex_);
    currentApp = activeApp_;
  }

  std::lock_guard<std::mutex> macroLock(macrosMutex_);
  auto appIt = appMacros_.find(currentApp);
  if (appIt == appMacros_.end()) {
    // logToFile("No macros for app: " +
    // std::to_string(static_cast<int>(currentApp)), LOG_MACROS);
    return PipelineResult::CONTINUE;
  }

  bool eventMatchedAnySuppressedStep = false;
  auto &comboMap = comboProgress_[currentApp];

  for (size_t comboIdx = 0; comboIdx < appIt->second.size(); ++comboIdx) {
    const KeyAction &action = appIt->second[comboIdx];
    if (action.trigger.events.empty())
      continue;

    ComboState &state = comboMap[comboIdx];

    if (numLockActive_ && !isKeyboardOnlyMacro(action)) {
      // If NumLock is active and this macro is not keyboard-only, skip it.
      // If there's an ongoing combo for this macro, it should be broken.
      if (state.nextKeyIndex > 0) {
        logToFile("Combo " + std::to_string(comboIdx) + " broken by NumLock ON",
                  LOG_MACROS);
        auto claimed = suppressionRegistry_.claim(comboIdx);
        for (const auto &wk : claimed) {
          if (!suppressionRegistry_.isBlocked(wk.code))
            emit(wk.type, wk.code, wk.value);
        }
        sync();
        state.suppressedEvents.clear();
        state.nextKeyIndex = 0;
      }
      continue;
    }

    if (state.nextKeyIndex >= action.trigger.events.size())
      continue; // Combo already completed or not started

    const auto &expectedTuple = action.trigger.events[state.nextKeyIndex];
    uint16_t expectedType = std::get<0>(expectedTuple);
    uint16_t expectedCode = std::get<1>(expectedTuple);
    int32_t expectedValue = std::get<2>(expectedTuple);
    bool shouldSuppress = std::get<3>(expectedTuple);
    bool ignoreRepeat = std::get<4>(expectedTuple);

    bool match = false;
    if (expectedType == ev.type && expectedCode == ev.code) {
      if (ev.type == EV_KEY) {
        if (((expectedValue == 1 && (ev.value == 1 || ev.value == 2)) ||
             (expectedValue == 0 && ev.value == 0)))
          match = true;
      } else if (expectedValue == ev.value)
        match = true;
    }

    if (match) {
      state.nextKeyIndex++;
      state.lastMatchTime = std::chrono::steady_clock::now();

      if (shouldSuppress) {
        eventMatchedAnySuppressedStep = true;
        state.suppressedEvents.push_back({ev.type, ev.code, ev.value});
        suppressionRegistry_.add(ev.type, ev.code, ev.value, comboIdx);
      }

      logToFile("Combo " + std::to_string(comboIdx) +
                    " progress: " + std::to_string(state.nextKeyIndex) + "/" +
                    std::to_string(action.trigger.events.size()),
                LOG_MACROS);

      if (state.nextKeyIndex == action.trigger.events.size()) {
        logToFile("COMBO COMPLETE: " + action.logMessage, LOG_AUTOMATION);
        suppressionRegistry_.claim(comboIdx);
        state.suppressedEvents.clear();
        executeKeyAction(action);
        state.nextKeyIndex = 0;
      }
    } else if (state.nextKeyIndex > 0) {
      bool shouldBreak = false;
      if (ev.type == EV_KEY) {
        bool isKeyInActiveCombo = false;
        for (size_t i = 0; i < state.nextKeyIndex; ++i) {
          if (std::get<1>(action.trigger.events[i]) == ev.code) {
            isKeyInActiveCombo = true;
            break;
          }
        }
        if (ev.value == 1 && action.trigger.noiseBreaks)
          shouldBreak = true;
        else if (ev.value == 0 && isKeyInActiveCombo)
          shouldBreak = true;
        else if (ev.value == 2 && !isKeyInActiveCombo && !ignoreRepeat)
          shouldBreak = true;
      }

      if (shouldBreak) {
        logToFile("Combo " + std::to_string(comboIdx) + " broken by " +
                      std::to_string(ev.code),
                  LOG_MACROS);
        auto claimed = suppressionRegistry_.claim(comboIdx);
        for (const auto &wk : claimed) {
          if (!suppressionRegistry_.isBlocked(wk.code))
            emit(wk.type, wk.code, wk.value);
        }
        sync();
        state.suppressedEvents.clear();
        state.nextKeyIndex = 0;
      }
    }
  }

  if (eventMatchedAnySuppressedStep)
    return PipelineResult::DROP;

  if (ev.type == EV_KEY && suppressionRegistry_.isBlocked(ev.code)) {
    return PipelineResult::DROP;
  }
  return PipelineResult::CONTINUE;
}

std::optional<GKey> InputMapper::detectGKey(const struct input_event &ev) {

  if (ev.type != EV_KEY) {
    return std::nullopt;
  }

  if (ev.code == KEY_LEFTCTRL) {
    if (ev.value == 1) {
      if (gToggleState_ == 1) {
        gToggleState_ = 2;
        logToFile("G-Key State: 1 -> 2 (Ctrl Down)", LOG_MACROS);
      } else if (gToggleState_ == 3) {
        gToggleState_ = 4;
        logToFile("G-Key State: 3 -> 4 (Ctrl Down)", LOG_MACROS);
      } else {
        gToggleState_ = 1;
      }
    }
    return std::nullopt;
  }

  if (ev.code == KEY_LEFTSHIFT) {
    if (ev.value == 1) {
      if (gToggleState_ == 2) {
        gToggleState_ = 3;
        logToFile("G-Key State: 2 -> 3 (Shift Down)", LOG_MACROS);
      } else if (gToggleState_ == 4) {
        gToggleState_ = 5;
        logToFile("G-Key State: 4 -> 5 (Shift Down)", LOG_MACROS);
      } else {
        gToggleState_ = 1;
      }
    }
    return std::nullopt;
  }

  // Check if we're in the ready state (gToggleState_ == 5)
  if (gToggleState_ == 5) {
    if (ev.code >= KEY_1 && ev.code <= KEY_6) {
      int gKeyNum = ev.code - KEY_1 + 1; // Convert KEY_1..KEY_6 to 1..6
      gToggleState_ = 1;                 // Reset state after G-key is detected
      return static_cast<GKey>(gKeyNum);
    }
    // Not a G-key number, reset state
    gToggleState_ = 1;
    return std::nullopt;
  }

  // Not in G-key sequence, reset state if needed
  if (gToggleState_ != 1) {
    logToFile("G-Key State: Reset from " + std::to_string(gToggleState_) +
                  " due to key code: " + std::to_string(ev.code),
              LOG_MACROS);
  }
  gToggleState_ = 1;
  return std::nullopt;
}

void InputMapper::executeKeyAction(const KeyAction &action) {
  logToFile(action.logMessage, LOG_AUTOMATION);
  if (action.customHandler) {
    action.customHandler(); // Call async handler (e.g., Chrome ChatGPT focus)
  } else {
    emitSequence(action.keySequence); // Emit key sequence
  }
}

void InputMapper::onFocusAck() {
  logToFile("[InputMapper] Focus ACK received, signaling CV", LOG_AUTOMATION);
  focusAckReceived_ = true;
  focusAckCv_.notify_all();
}

void InputMapper::triggerChromeChatGPTMacro() {
  extern void triggerChromeChatGPTFocus();
  logToFile("[InputMapper] Triggering ChatGPT focus macro", LOG_AUTOMATION);

  // Clear previous state
  focusAckReceived_ = false;

  triggerChromeChatGPTFocus();

  // Wait for ACK in a separate thread so we don't block the input loop
  std::thread([this]() {
    {
      std::unique_lock<std::mutex> lock(focusAckMutex_);
      // Wait for up to 400ms for the extension to respond
      bool received =
          focusAckCv_.wait_for(lock, std::chrono::milliseconds(400),
                               [this] { return (bool)focusAckReceived_; });

      if (received) {
        logToFile("[InputMapper] Focus ACK confirmed via CV, pasting NOW",
                  LOG_AUTOMATION);
      } else {
        logToFile("[InputMapper] Focus ACK TIMEOUT (400ms), pasting anyway",
                  LOG_AUTOMATION);
      }
    }

    // Small extra safety delay to ensure the browser has processed the
    // focus event internally AND to prevent window-switch race
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    emitSequence(
        {{KEY_LEFTCTRL, 1}, {KEY_V, 1}, {KEY_V, 0}, {KEY_LEFTCTRL, 0}});
    sync(); // Ensure sequence is flushed
  }).detach();
}
void InputMapper::flushAndResetState() {
  logToFile("[InputMapper] Emergency Flush & Reset (Sync Triggered)", LOG_CORE);

  std::lock_guard<std::mutex> macroLock(macrosMutex_);
  // Clear all combo progress
  for (auto &appPair : comboProgress_) {
    for (auto &comboPair : appPair.second) {
      ComboState &state = comboPair.second;
      auto claimed = suppressionRegistry_.claim(comboPair.first);
      for (const auto &wk : claimed) {
        emit(wk.type, wk.code, wk.value);
      }
      state.suppressedEvents.clear();
      state.nextKeyIndex = 0;
    }
  }

  // Reset G-key toggle state
  gToggleState_ = 1;

  logToFile("[InputMapper] Reset complete.", LOG_CORE);
}

void InputMapper::setNumLockState(bool active) {
  if (numLockActive_ != active) {
    numLockActive_ = active;
    logToFile(std::string("NumLock changed: ") +
                  (numLockActive_ ? "ON (UNGRAB - Macros Disabled)"
                                  : "OFF (GRAB - Macros Enabled)"),
              LOG_CORE);

    // Core improvement: ungrab devices when NumLock is ON
    // and request a grab when NumLock is OFF.
    if (numLockActive_) {
      ungrabDevices();
    } else {
      grabDevices();
    }

    // Release any stuck keys on toggle
    releaseAllPressedKeys();
  }
}

bool InputMapper::isKeyboardOnlyMacro(const KeyAction &action) const {
  // A macro is "keyboard only" if all its trigger keys and target sequence
  // keys are standard keyboard keys.
  for (const auto &eventTuple : action.trigger.events) {
    uint16_t type = std::get<0>(eventTuple);
    uint16_t code = std::get<1>(eventTuple);
    if (type != EV_KEY)
      return false;
    if (code >= BTN_MISC && code < KEY_OK)
      return false; // Mouse buttons area
    if (code >= 1000)
      return false; // Virtual G-keys
  }
  for (const auto &sq : action.keySequence) {
    uint16_t code = sq.first;
    if (code >= BTN_MISC && code < KEY_OK)
      return false;
    if (code >= 1000)
      return false;
  }
  return true;
}

void InputMapper::releaseAllPressedKeys() {
  logToFile("Context change: releasing all keys and resetting combos",
            LOG_CORE);

  // 1. Flush all registries
  std::lock_guard<std::mutex> macroLock(macrosMutex_);
  for (auto &appPair : comboProgress_) {
    for (auto &comboPair : appPair.second) {
      ComboState &state = comboPair.second;
      suppressionRegistry_.claim(comboPair.first);
      state.suppressedEvents.clear();
      state.nextKeyIndex = 0;
    }
  }

  // 2. Physical release
  {
    std::lock_guard<std::mutex> keyLock(pressedKeysMutex_);
    for (uint16_t code : pressedKeys_) {
      emit(EV_KEY, code, 0);
    }
    sync();
    pressedKeys_.clear();
  }
}

std::string InputMapper::formatEvent(const struct input_event &ev,
                                     const std::string &devicePath) const {
  std::string typeStr;
  std::string codeStr;

  if (ev.type == EV_KEY) {
    const char *name = libevdev_event_code_get_name(ev.type, ev.code);
    if (name) {
      string n(name);
      if (n.find("BTN_") == 0) {
        typeStr = "btn";
        codeStr = n.substr(4);
      } else {
        typeStr = "key";
        codeStr = (n.find("KEY_") == 0) ? n.substr(4) : n;
      }
      std::transform(codeStr.begin(), codeStr.end(), codeStr.begin(),
                     ::tolower);
    } else {
      typeStr = "key";
      codeStr = std::to_string(ev.code);
    }
  } else if (ev.type == EV_REL) {
    typeStr = "rel";
    const char *name = libevdev_event_code_get_name(ev.type, ev.code);
    if (name) {
      codeStr = name;
      // Strip "REL_" prefix and convert to lowercase
      if (codeStr.find("REL_") == 0) {
        codeStr = codeStr.substr(4);
      }
      std::transform(codeStr.begin(), codeStr.end(), codeStr.begin(),
                     ::tolower);
    } else {
      codeStr = std::to_string(ev.code);
    }
  } else if (ev.type == EV_ABS) {
    typeStr = "abs";
    const char *name = libevdev_event_code_get_name(ev.type, ev.code);
    if (name) {
      codeStr = name;
      if (codeStr.find("ABS_") == 0) {
        codeStr = codeStr.substr(4);
      }
      std::transform(codeStr.begin(), codeStr.end(), codeStr.begin(),
                     ::tolower);
    } else {
      codeStr = std::to_string(ev.code);
    }
  } else if (ev.type == EV_MSC) {
    typeStr = "msc";
    const char *name = libevdev_event_code_get_name(ev.type, ev.code);
    if (name) {
      codeStr = name;
      if (codeStr.find("MSC_") == 0) {
        codeStr = codeStr.substr(4);
      }
      std::transform(codeStr.begin(), codeStr.end(), codeStr.begin(),
                     ::tolower);
    } else {
      codeStr = std::to_string(ev.code);
    }
  } else {
    return ""; // Skip other types (SYN, etc.) for this format or handle
               // differently
  }

  return typeStr + ":" + codeStr + ":" + std::to_string(ev.value) + "@" +
         devicePath;
}

void InputMapper::emitFinal(const struct input_event &ev,
                            const std::string &devicePath) {
  (void)devicePath;
  if (ev.type == EV_REL || ev.type == EV_ABS) {
    std::lock_guard<std::mutex> lock(uinputMutex_);
    if (uinputDev_) {
      // Direct write for mouse movement to preserve hardware grouping
      libevdev_uinput_write_event(uinputDev_, ev.type, ev.code, ev.value);
    }
  } else {
    // Standard emit for everything else (keys, buttons, syn)
    emit(ev.type, ev.code, ev.value);
  }
}

bool InputMapper::shouldLog(const std::string &eventStr, uint32_t category) {
  // 1. Check global category bitmask
  extern unsigned int shouldLog;
  if (!(shouldLog & category)) {
    return false;
  }

  // 2. Check granular string filters
  std::lock_guard<std::mutex> lock(filtersMutex_);
  if (logFilterPatterns_.empty()) {
    return true; // No filters means allowed
  }

  for (const auto &pattern : logFilterPatterns_) {
    if (eventStr.find(pattern) != std::string::npos) {
      return true;
    }
  }
  return false;
}
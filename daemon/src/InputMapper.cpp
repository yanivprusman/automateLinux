#include "InputMapper.h"
#include "Constants.h"
#include "Globals.h"
#include "KVTable.h"
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
#include <thread>
#include <unistd.h>
#include <vector>

using namespace std;

InputMapper::InputMapper() { initializeAppMacros(); }

InputMapper::~InputMapper() { stop(); }

void InputMapper::loadPersistence() {
  // Load Macros from DB
  string savedMacros = kvTable.get("custom_macros");
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
  string savedFilters = kvTable.get("custom_event_filters");
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
    // libevdev_grab(keyboardDev_, LIBEVDEV_UNGRAB); // Removed, handled by ungrabDevices()
    libevdev_free(keyboardDev_);
    keyboardDev_ = nullptr;
  }
  if (mouseDev_) {
    // libevdev_grab(mouseDev_, LIBEVDEV_UNGRAB); // Removed, handled by ungrabDevices()
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
      logToFile("DEBUG_KEYS: Initial pressed key: " + std::to_string(code), LOG_MACROS);
    }
  }
  logToFile("DEBUG_KEYS: Initial pressed keys count: " + std::to_string(pressedKeys_.size()), LOG_MACROS);



  if (!mousePath_.empty()) {
    mouseFd_ = open(mousePath_.c_str(), O_RDONLY | O_NONBLOCK);
    if (mouseFd_ >= 0) {
      if (libevdev_new_from_fd(mouseFd_, &mouseDev_) < 0) {
        logToFile("Failed to initialize libevdev for mouse", LOG_CORE);
      } else {

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
      for (const auto &tk : action.trigger.keyCodes) {
        json k;
        k["code"] = std::get<0>(tk);
        k["state"] = std::get<1>(tk);
        k["suppress"] = std::get<2>(tk);
        triggerKeys.push_back(k);
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
  return j;
}

json InputMapper::getEventFiltersJson() {
  std::lock_guard<std::mutex> lock(filtersMutex_);
  json j = json::array();
  for (uint16_t code : filteredKeyCodes_) {
    j.push_back(code);
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
      if (ma.contains("trigger") && ma["trigger"].contains("keys")) {
        for (const auto &tk : ma["trigger"]["keys"]) {
          if (tk.is_array() && tk.size() >= 2) {
            uint16_t code = tk[0].get<uint16_t>();
            uint8_t state = tk[1].get<uint8_t>();
            bool suppress = tk.size() >= 3 ? tk[2].get<bool>() : false;
            trigger.keyCodes.push_back({code, state, suppress});
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

void InputMapper::setMacrosFromJson(const json &j) {
  {
    std::lock_guard<std::mutex> lock(macrosMutex_);
    setMacrosFromJsonInternal(j);
  }
  kvTable.upsert("custom_macros", j.dump());
  logToFile("Macros updated dynamically and saved to DB", LOG_CORE);
}

void InputMapper::setEventFiltersInternal(const json &j) {
  filteredKeyCodes_.clear();
  if (j.is_array()) {
    for (const auto &item : j) {
      filteredKeyCodes_.insert(item.get<uint16_t>());
    }
  }
}

void InputMapper::setEventFilters(const json &j) {
  {
    std::lock_guard<std::mutex> lock(filtersMutex_);
    setEventFiltersInternal(j);
  }
  kvTable.upsert("custom_event_filters", j.dump());
  logToFile("Event filters updated dynamically and saved to DB (count: " +
                std::to_string(filteredKeyCodes_.size()) + ")",
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
    // Check for pending grab
    if (pendingGrab_ && pressedKeys_.empty()) {
      logToFile("InputMapper: All keys released, performing deferred grab.", LOG_CORE);
      grabDevices();
      pendingGrab_ = false;
    }
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
            processEvent(ev, true, true, keyboardPath_); // true for sync (skip macros)
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

void InputMapper::setPendingGrab(bool value) {
  pendingGrab_ = value;
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
  if (keyboardDev_) {
    if (libevdev_grab(keyboardDev_, LIBEVDEV_GRAB) < 0) {
      logToFile("Failed to grab keyboard", LOG_CORE);
    } else {
      logToFile("InputMapper: Keyboard grabbed successfully", LOG_CORE);
    }
  }
  if (mouseDev_) {
    if (libevdev_grab(mouseDev_, LIBEVDEV_GRAB) < 0) {
      logToFile("Failed to grab mouse", LOG_CORE);
    } else {
      logToFile("InputMapper: Mouse grabbed successfully", LOG_CORE);
    }
  }
  monitoringMode_ = false; // We are no longer just monitoring
}

void InputMapper::ungrabDevices() {
  if (keyboardDev_) {
    libevdev_grab(keyboardDev_, LIBEVDEV_UNGRAB);
    logToFile("InputMapper: Keyboard ungrabbed", LOG_CORE);
  }
  if (mouseDev_) {
    libevdev_grab(mouseDev_, LIBEVDEV_UNGRAB);
    logToFile("InputMapper: Mouse ungrabbed", LOG_CORE);
  }
  monitoringMode_ = true; // We are back to monitoring (if devices are still open)
}

void InputMapper::setContext(AppType appType, const std::string &url,
                             const std::string &title) {
  std::lock_guard<std::mutex> lock(contextMutex_);
  activeApp_ = appType;
  activeUrl_ = url;
  activeTitle_ = title;
  logToFile("Context updated: App=[" + appTypeToString(activeApp_) + "] URL=[" +
                activeUrl_ + "] Title=[" + activeTitle_ + "]",
            LOG_WINDOW);
}

void InputMapper::processEvent(struct input_event &ev, bool isKeyboard,
                               bool skipMacros, const std::string& devicePath) {
  logToFile("DEBUG_EV: Type=" + std::to_string(ev.type) +
            ", Code=" + std::to_string(ev.code) +
            ", Value=" + std::to_string(ev.value) +
            ", isKBD=" + std::to_string(isKeyboard) +
            ", skipMacros=" + std::to_string(skipMacros) +
            ", Device=" + devicePath, LOG_MACROS);

  // NEW KEY TRACKING LOGIC
  if (ev.type == EV_KEY) {
    if (ev.value == 1) { // Key press
      pressedKeys_.insert(ev.code);
      logToFile("DEBUG_KEYS: Key " + std::to_string(ev.code) + " pressed. Total: " + std::to_string(pressedKeys_.size()), LOG_MACROS);
    } else if (ev.value == 0) { // Key release
      pressedKeys_.erase(ev.code);
      logToFile("DEBUG_KEYS: Key " + std::to_string(ev.code) + " released. Total: " + std::to_string(pressedKeys_.size()), LOG_MACROS);
    }
  }
  // Update LeftCtrl state for macros (tracked outside Chrome block)
  if (isKeyboard && ev.type == EV_KEY && ev.code == KEY_LEFTCTRL) {
    ctrlDown_ = (ev.value != 0);
  }

  // Filter out MSC events to prevent interference (NOT ANYMORE - let them pass)
  // if (ev.type == EV_MSC) {
  //   return;
  // }

  // Debug: Log all Ctrl+V combinations to see current context
  if (!skipMacros && isKeyboard && ev.type == EV_KEY && ev.code == KEY_V &&
      ctrlDown_ && ev.value == 1) {
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

  // 3. G-Key sequence detection
  if (!skipMacros && isKeyboard) {
    auto gKey = detectGKey(ev);
    if (gKey) {
      logToFile("VIRTUAL G-KEY DETECTED: G" +
                    std::to_string(static_cast<int>(*gKey)),
                LOG_AUTOMATION);
      struct input_event virtualEv = ev;
      virtualEv.code =
          1000 + static_cast<uint16_t>(*gKey); // Map to G1_VIRTUAL..G6_VIRTUAL
      processEvent(virtualEv, true, false, keyboardPath_);
      return;
    }
  }

  // === NEW PARALLEL COMBO MATCHING LOGIC ===
  // On key press or release: test all active combos and track progress
  if (!skipMacros && ev.type == EV_KEY) {
    // Skip macro logic for basic mouse buttons (Left, Right, Middle)
    // to prevent keyboard macros from blocking mouse clicks.
    // We still allow BTN_FORWARD as it's used for our 'Enter' macro.
    bool isStandardMouseButton =
        !isKeyboard &&
        (ev.code == BTN_LEFT || ev.code == BTN_RIGHT || ev.code == BTN_MIDDLE);

    if (!isStandardMouseButton) {
      AppType currentApp;
      {
        std::lock_guard<std::mutex> lock(contextMutex_);
        currentApp = activeApp_;
      }

      std::lock_guard<std::mutex> macroLock(macrosMutex_);
      auto appIt = appMacros_.find(currentApp);
      if (appIt != appMacros_.end()) {
        logToFile("DEBUG_MACRO: Found " + std::to_string(appIt->second.size()) +
                  " macros for app: " + appTypeToString(currentApp), LOG_MACROS);
        bool currentlySuppressing = false;
        bool eventConsumed = false;

        bool shouldSuppressThisSpecificEvent = false;

        // Test all combos for this app
        for (size_t comboIdx = 0; comboIdx < appIt->second.size(); ++comboIdx) {
          const KeyAction &action = appIt->second[comboIdx];
          if (action.trigger.keyCodes.empty())
            continue;

          auto &comboMap = comboProgress_[currentApp];
          ComboState &state = comboMap[comboIdx];

          if (state.nextKeyIndex < action.trigger.keyCodes.size()) {
            const auto &expectedKeyTuple =
                action.trigger.keyCodes[state.nextKeyIndex];
            uint16_t expectedCode = std::get<0>(expectedKeyTuple);
            uint8_t expectedState = std::get<1>(expectedKeyTuple);
            bool shouldSuppress = std::get<2>(expectedKeyTuple);

            if (expectedCode == ev.code &&
                ((expectedState == 1 && (ev.value == 1 || ev.value == 2)) ||
                 (expectedState == 0 && ev.value == 0))) {
              // Match! Advance state
              state.nextKeyIndex++;
              eventConsumed = true; // Key matched a combo step

              if (shouldSuppress) {
                shouldSuppressThisSpecificEvent = true;
                state.suppressedKeys.push_back({ev.code, (uint8_t)ev.value});
              }

              logToFile("Combo " + std::to_string(comboIdx) + " progress: " +
                            std::to_string(state.nextKeyIndex) + "/" +
                            std::to_string(action.trigger.keyCodes.size()),
                        LOG_MACROS);

              // Check if combo is complete
              if (state.nextKeyIndex == action.trigger.keyCodes.size()) {
                logToFile("COMBO COMPLETE: " + action.logMessage,
                          LOG_AUTOMATION);

                if (action.trigger.hasSuppressedKeys) {
                  // Consume trigger from queue (the keys that match the combo)
                  std::lock_guard<std::mutex> lock(pendingEventsMutex_);
                  // We only want to remove events that we actually suppressed
                  // for THIS combo
                  for (const auto &sk : state.suppressedKeys) {
                    auto it = std::find_if(
                        pendingEvents_.begin(), pendingEvents_.end(),
                        [&sk](const PendingEvent &pe) {
                          return pe.code == sk.first && pe.value == sk.second;
                        });
                    if (it != pendingEvents_.end()) {
                      pendingEvents_.erase(it);
                    }
                  }
                  state.suppressedKeys.clear();
                }

                executeKeyAction(action);
                state.nextKeyIndex = 0;
              }
            } else if (state.nextKeyIndex > 0) {
              // The current event 'ev' did not match the next expected step in the combo.
              // We need to decide if this event should break the combo.

              bool shouldBreak = false;
              if (ev.type == EV_KEY) {
                  if (ev.value == 1) { // A new key was pressed. If it's not the expected key, it breaks.
                      shouldBreak = true;
                  } else if (ev.value == 0) { // A key was released. If it was a key required for a previous combo step, it breaks.
                      // Check if the released key was part of the combo's already matched steps.
                      bool wasPreviouslyMatchedKey = false;
                      for (size_t i = 0; i < state.nextKeyIndex; ++i) {
                          if (std::get<0>(action.trigger.keyCodes[i]) == ev.code) {
                              wasPreviouslyMatchedKey = true;
                              break;
                          }
                      }
                      if (wasPreviouslyMatchedKey) {
                          shouldBreak = true;
                      }
                  }
                  // Key repeats (ev.value == 2) do not break a combo if they are not the next expected step.
              } else {
                  // Non-key events (e.g., EV_REL, EV_ABS) currently do not break combos.
                  // This might need refinement for more complex scenarios, but for now, keep it simple.
              }

              if (shouldBreak) {
                logToFile("Combo " + std::to_string(comboIdx) +
                                " broken at step " +
                                std::to_string(state.nextKeyIndex) +
                                " by non-matching event (code=" + std::to_string(ev.code) +
                                ", value=" + std::to_string(ev.value) + ")",
                                LOG_MACROS);
                state.nextKeyIndex = 0;
                state.suppressedKeys.clear();
              }
          }
        }

        // Re-evaluate if any combo is still in progress
        bool anyComboInProgress = false;
        for (const auto &pair : comboProgress_[currentApp]) {
          if (pair.second.nextKeyIndex > 0) {
            anyComboInProgress = true;
            break;
          }
        }

        if (ev.code == KEY_ENTER || ev.code == KEY_KPENTER) {
          extern void forceLog(const std::string &message);
          forceLog(
              "ENTER DEBUG [1]: Consumed=" + std::to_string(eventConsumed) +
              " AnyCombo=" + std::to_string(anyComboInProgress));
        }

        if (eventConsumed) {
          if (shouldSuppressThisSpecificEvent) {
            // Queue this event
            std::lock_guard<std::mutex> lock(pendingEventsMutex_);
            pendingEvents_.push_back(
                {(uint16_t)ev.type, (uint16_t)ev.code, (int32_t)ev.value});
          } else {
            // Not suppressed by any matching combo. Emit now!
            emit(ev.type, ev.code, ev.value);
          }

          // IMPORTANT: Always check for flush after an event is consumed,
          // especially if it completed a macro.
          if (!anyComboInProgress) {
            std::lock_guard<std::mutex> lock(pendingEventsMutex_);
            if (!pendingEvents_.empty()) {
              logToFile("Flushing remaining after complete (" +
                            std::to_string(pendingEvents_.size()) + ")",
                        LOG_MACROS);
              for (const auto &pe : pendingEvents_) {
                emit(pe.type, pe.code, pe.value);
              }
              sync(); // Sync after flushing unblocked queue
              pendingEvents_.clear();
            }
          }
          return;
        }

        // If we are currently in a suppressed state (holding a trigger key), we
        // must queue even unmatched keys to preserve order.
        {
          std::lock_guard<std::mutex> lock(pendingEventsMutex_);
          currentlySuppressing = !pendingEvents_.empty();
        }

        if (ev.code == KEY_ENTER || ev.code == KEY_KPENTER) {
          extern void forceLog(const std::string &message);
          forceLog("ENTER DEBUG [2]: Suppressing=" +
                   std::to_string(currentlySuppressing) +
                   " AnyCombo=" + std::to_string(anyComboInProgress));
        }

        // Critical unblocking: ENTER and KPENTER should never be stuck in a
        // queue if they are not part of an active macro.
        if (!anyComboInProgress &&
            (ev.code == KEY_ENTER || ev.code == KEY_KPENTER)) {
          std::lock_guard<std::mutex> lock(pendingEventsMutex_);
          if (!pendingEvents_.empty()) {
            logToFile("Unblocking queue via ENTER key (" +
                          std::to_string(pendingEvents_.size()) + " queued)",
                      LOG_MACROS);
            for (const auto &pe : pendingEvents_) {
              emit(pe.type, pe.code, pe.value);
            }
            pendingEvents_.clear();
            sync(); // Sync after flushing unblocked queue
          }
        }

        if (currentlySuppressing && anyComboInProgress) {
          // Still matching some other potential combo, keep queueing
          std::lock_guard<std::mutex> lock(pendingEventsMutex_);
          pendingEvents_.push_back(
              {(uint16_t)ev.type, (uint16_t)ev.code, (int32_t)ev.value});
          return;
        } else {
          // Not matching anything anymore, or we weren't suppressing.
          std::lock_guard<std::mutex> lock(pendingEventsMutex_);
          if (!pendingEvents_.empty()) {
            logToFile("Flushing pending events (mismatch/broken combo: " +
                          std::to_string(pendingEvents_.size()) + ")",
                      LOG_MACROS);
            for (const auto &pe : pendingEvents_) {
              emit(pe.type, pe.code, pe.value);
            }
            pendingEvents_.clear();
            sync(); // Sync after flushing unblocked queue
          }
        }
      }
    }
  }

  // Filter Logic:
  // 1. If filters are empty -> Log ALL keys/buttons
  // 2. If filters exist -> Log ONLY matching keys
  bool shouldLogInput = false;
  if (ev.type == EV_KEY) {
    std::lock_guard<std::mutex> lock(filtersMutex_);
    if (filteredKeyCodes_.empty()) {
      shouldLogInput = true; // No filter = Log everything
    } else {
      shouldLogInput = (filteredKeyCodes_.count(ev.code) > 0);
    }
  }

  if (shouldLogInput) {
    logToFile(std::string(isKeyboard ? "KBD" : "MOUSE") +
                  " Processing event (type " + std::to_string(ev.type) +
                  ", code " + std::to_string(ev.code) + ", value " +
                  std::to_string(ev.value) + ") -> uinput",
              LOG_INPUT);
  }

  // Use emit() for keys and mouse movements to ensure immediate EV_SYN.
  // This prevents the kernel from delaying events until the next physical
  // sync.
  if (ev.type == EV_KEY || ev.type == EV_REL || ev.type == EV_ABS) {
    emit(ev.type, ev.code, ev.value);
  } else {
    // Detailed logging for MSC/SYN events when they follow or precede Enters
    if (ev.type == EV_MSC || ev.type == EV_SYN) {
      if (ev.type == EV_SYN) {
        logToFile("--- EV_SYN ---", LOG_INPUT);
      } else { // ev.type == EV_MSC
        logToFile("msc:scan:" + std::to_string(ev.value) + "@" + devicePath,
                  LOG_INPUT);
      }
    }

    // Pure synchronization or metadata events pass through as-is
    std::lock_guard<std::mutex> lock(uinputMutex_);
    int rc =
        libevdev_uinput_write_event(uinputDev_, ev.type, ev.code, ev.value);
    if (rc < 0) {
      logToFile(
          "Failed to write to uinput (raw type=" + std::to_string(ev.type) +
              "): " + std::string(strerror(-rc)),
          LOG_INPUT);
    }
  }
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

    // Small extra safety delay to ensure the browser has processed the focus
    // event internally AND to prevent window-switch race
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    emitSequence(
        {{KEY_LEFTCTRL, 1}, {KEY_V, 1}, {KEY_V, 0}, {KEY_LEFTCTRL, 0}});
    sync(); // Ensure sequence is flushed
  }).detach();
}
void InputMapper::flushAndResetState() {
  logToFile("[InputMapper] Emergency Flush & Reset (Sync Triggered)", LOG_CORE);

  // 1. Emit all pending events
  {
    std::lock_guard<std::mutex> lock(pendingEventsMutex_);
    if (!pendingEvents_.empty()) {
      logToFile("Flushing " + std::to_string(pendingEvents_.size()) +
                    " events during Sync",
                LOG_MACROS);
      for (const auto &pe : pendingEvents_) {
        emit(pe.type, pe.code, pe.value);
      }
      sync(); // Final sync after flush
      pendingEvents_.clear();
    }
  }

  // 2. Reset ALL combo progress for ALL apps
  // Note: This is a heavy-handed reset to ensure we don't stay in a "stuck"
  // suppress state.
  for (auto &appPair : comboProgress_) {
    for (auto &comboPair : appPair.second) {
      comboPair.second.nextKeyIndex = 0;
      comboPair.second.suppressedKeys.clear();
    }
  }

  // 3. Reset G-key toggle state
  gToggleState_ = 1;

  logToFile("[InputMapper] Reset complete.", LOG_CORE);
}

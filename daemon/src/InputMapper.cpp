#include "InputMapper.h"
#include "Constants.h"
#include "Globals.h"
#include "Utils.h"
#include <atomic>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <linux/input-event-codes.h>
#include <mutex>
#include <poll.h>
#include <unistd.h>
#include <vector>

InputMapper::InputMapper() { initializeAppMacros(); }

InputMapper::~InputMapper() { stop(); }

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

  if (libevdev_grab(keyboardDev_, LIBEVDEV_GRAB) < 0) {
    std::cerr << "CRITICAL: Failed to grab keyboard: " << strerror(errno)
              << std::endl;
    logToFile("Failed to grab keyboard", LOG_CORE);
    return false;
  }
  logToFile("InputMapper: Keyboard grabbed successfully", LOG_CORE);

  if (!mousePath_.empty()) {
    mouseFd_ = open(mousePath_.c_str(), O_RDONLY | O_NONBLOCK);
    if (mouseFd_ >= 0) {
      if (libevdev_new_from_fd(mouseFd_, &mouseDev_) < 0) {
        logToFile("Failed to initialize libevdev for mouse", LOG_CORE);
      } else {
        if (libevdev_grab(mouseDev_, LIBEVDEV_GRAB) < 0) {
          logToFile("Failed to grab mouse", LOG_CORE);
        }
      }
    }
  }

  return true;
}

bool InputMapper::setupUinput() {
  struct libevdev *uinput_template = libevdev_new();
  libevdev_set_name(uinput_template, "AutomateLinux Virtual Device");

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
          while (libevdev_next_event(keyboardDev_, LIBEVDEV_READ_FLAG_SYNC,
                                     &ev) == LIBEVDEV_READ_STATUS_SYNC) {
            processEvent(ev, true, true); // true for sync (skip macros)
          }
          continue;
        }
        processEvent(ev, true, false);
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
          while (libevdev_next_event(mouseDev_, LIBEVDEV_READ_FLAG_SYNC, &ev) ==
                 LIBEVDEV_READ_STATUS_SYNC) {
            processEvent(ev, false, true); // true for sync
          }
          continue;
        }
        processEvent(ev, false, false);
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

void InputMapper::emitSequence(
    const std::vector<std::pair<uint16_t, int32_t>> &sequence) {
  if (!uinputDev_)
    return;
  std::lock_guard<std::mutex> lock(uinputMutex_);
  for (const auto &p : sequence) {
    libevdev_uinput_write_event(uinputDev_, EV_KEY, p.first, p.second);
  }
  libevdev_uinput_write_event(uinputDev_, EV_SYN, SYN_REPORT, 0);
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
                               bool skipMacros) {
  // Update LeftCtrl state for macros (tracked outside Chrome block)
  if (isKeyboard && ev.type == EV_KEY && ev.code == KEY_LEFTCTRL) {
    ctrlDown_ = (ev.value != 0);
  }

  // Filter out MSC events to prevent interference
  if (ev.type == EV_MSC) {
    return;
  }

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
              LOG_INPUT);
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
      processEvent(virtualEv, true, false);
      return;
    }
  }

  // === NEW PARALLEL COMBO MATCHING LOGIC ===
  // On key press or release: test all active combos and track progress
  if (!skipMacros && ev.type == EV_KEY) { // Any key event
    AppType currentApp;
    {
      std::lock_guard<std::mutex> lock(contextMutex_);
      currentApp = activeApp_;
    }

    auto appIt = appMacros_.find(currentApp);
    if (appIt != appMacros_.end()) {
      bool currentlySuppressing = false;
      bool completedSuppressing = false;

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

          if (expectedCode == ev.code && expectedState == ev.value) {
            // Match! Advance state
            state.nextKeyIndex++;

            if (shouldSuppress) {
              state.suppressedKeys.push_back({ev.code, (uint8_t)ev.value});
            }

            logToFile("Combo " + std::to_string(comboIdx) +
                          " progress: " + std::to_string(state.nextKeyIndex) +
                          "/" + std::to_string(action.trigger.keyCodes.size()),
                      LOG_INPUT);

            // Check if combo is complete
            if (state.nextKeyIndex == action.trigger.keyCodes.size()) {
              logToFile("COMBO COMPLETE: " + action.logMessage, LOG_AUTOMATION);

              bool anySuppressed = false;
              for (const auto &keyTuple : action.trigger.keyCodes) {
                if (std::get<2>(keyTuple)) {
                  anySuppressed = true;
                  break;
                }
              }

              if (anySuppressed) {
                completedSuppressing = true;
                // Consume trigger from queue (the keys that match the combo)
                std::lock_guard<std::mutex> lock(pendingEventsMutex_);
                if (state.suppressedKeys.size() > 1) {
                  size_t toRemove = state.suppressedKeys.size() - 1;
                  if (pendingEvents_.size() >= toRemove) {
                    pendingEvents_.erase(pendingEvents_.begin(),
                                         pendingEvents_.begin() + toRemove);
                  }
                }
                state.suppressedKeys.clear();
              }

              executeKeyAction(action);
              state.nextKeyIndex = 0;
            }
          } else if (state.nextKeyIndex > 0) {
            // Broke an existing combo
            logToFile("Combo " + std::to_string(comboIdx) + " broken at step " +
                          std::to_string(state.nextKeyIndex),
                      LOG_INPUT);
            state.nextKeyIndex = 0;
            state.suppressedKeys.clear();
          }
        }
      }

      // Check if ANY combo is STILL in progress and suppressing
      for (const auto &pair : comboProgress_[currentApp]) {
        if (pair.first >= appIt->second.size())
          continue;
        const KeyAction &action = appIt->second[pair.first];
        if (pair.second.nextKeyIndex > 0) {
          // Check if any key in this combo at any position has suppress=true
          bool anySuppressed = false;
          for (const auto &keyTuple : action.trigger.keyCodes) {
            if (std::get<2>(keyTuple)) {
              anySuppressed = true;
              break;
            }
          }
          if (anySuppressed) {
            currentlySuppressing = true;
            break;
          }
        }
      }

      if (completedSuppressing) {
        // Current event finished a suppressing combo. Consume it.
        // It is NOT in pendingEvents_ yet.
        if (!currentlySuppressing) {
          // No other matching combos are suppressing. Replay the REST of the
          // queue.
          std::lock_guard<std::mutex> lock(pendingEventsMutex_);
          if (!pendingEvents_.empty()) {
            logToFile("Flushing " + std::to_string(pendingEvents_.size()) +
                          " pending events (after complete)",
                      LOG_INPUT);
            for (const auto &pe : pendingEvents_) {
              emit(pe.type, pe.code, pe.value);
            }
            pendingEvents_.clear();
          }
        }
        return; // Event consumed
      }

      if (currentlySuppressing) {
        // Queue this event
        std::lock_guard<std::mutex> lock(pendingEventsMutex_);
        pendingEvents_.push_back(
            {(uint16_t)ev.type, (uint16_t)ev.code, (int32_t)ev.value});
        return;
      } else {
        // Not suppressing anymore. If we have pending events, flush them.
        std::lock_guard<std::mutex> lock(pendingEventsMutex_);
        if (!pendingEvents_.empty()) {
          logToFile("Flushing " + std::to_string(pendingEvents_.size()) +
                        " pending events (after mismatch)",
                    LOG_INPUT);
          for (const auto &pe : pendingEvents_) {
            emit(pe.type, pe.code, pe.value);
          }
          pendingEvents_.clear();
        }
      }
    }
  }

  if (ev.type == EV_KEY && (ev.code == KEY_ENTER || ev.code == KEY_KPENTER)) {
    logToFile("Processing ENTER key (code " + std::to_string(ev.code) +
                  ", value " + std::to_string(ev.value) + ") -> uinput",
              LOG_INPUT);
  }

  std::lock_guard<std::mutex> lock(uinputMutex_);
  int rc = libevdev_uinput_write_event(uinputDev_, ev.type, ev.code, ev.value);
  if (rc < 0) {
    char errBuf[128];
    int len = snprintf(errBuf, sizeof(errBuf),
                       "Failed to write to uinput: %s (type=%d, code=%d)\n",
                       strerror(-rc), ev.type, ev.code);
    write(STDERR_FILENO, errBuf, len);
    logToFile("Failed to write to uinput: " + std::string(strerror(-rc)),
              LOG_INPUT);
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
        logToFile("G-Key State: 1 -> 2 (Ctrl Down)", LOG_INPUT);
      } else if (gToggleState_ == 3) {
        gToggleState_ = 4;
        logToFile("G-Key State: 3 -> 4 (Ctrl Down)", LOG_INPUT);
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
        logToFile("G-Key State: 2 -> 3 (Shift Down)", LOG_INPUT);
      } else if (gToggleState_ == 4) {
        gToggleState_ = 5;
        logToFile("G-Key State: 4 -> 5 (Shift Down)", LOG_INPUT);
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
              LOG_INPUT);
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

void InputMapper::triggerChromeChatGPTMacro() {
  extern void triggerChromeChatGPTFocus();
  triggerChromeChatGPTFocus();

  // Use a detached thread to fire the paste after a short delay
  std::thread([this]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    logToFile("[InputMapper] Async speculative paste fired", LOG_AUTOMATION);
    // Double sequence to ensure focus is solid
    emitSequence(
        {{KEY_LEFTCTRL, 1}, {KEY_V, 1}, {KEY_V, 0}, {KEY_LEFTCTRL, 0}});
  }).detach();
}

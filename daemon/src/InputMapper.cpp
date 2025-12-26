#include "InputMapper.h"
#include "Constants.h"
#include "Globals.h"
#include "Utils.h"
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <poll.h>
#include <unistd.h>
#include <vector>

InputMapper::InputMapper() {}

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
  running_ = false;
  if (thread_.joinable()) {
    thread_.join();
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
  std::cerr << "InputMapper: Keyboard grabbed successfully" << std::endl;

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

  std::cerr << "InputMapper loop starting..." << std::endl;
  while (running_) {
    int rc = poll(fds, nfds, 100); // 100ms timeout
    if (rc < 0)
      break;
    if (rc == 0)
      continue;

    if (fds[0].revents & POLLIN) {
      struct input_event ev;
      while (libevdev_next_event(keyboardDev_, LIBEVDEV_READ_FLAG_NORMAL,
                                 &ev) == LIBEVDEV_READ_STATUS_SUCCESS) {
        logToFile("KBD Event: type=" + std::to_string(ev.type) +
                      " code=" + std::to_string(ev.code) +
                      " value=" + std::to_string(ev.value),
                  LOG_INPUT);
        processEvent(ev, true);
      }
    }

    if (mouseFd_ >= 0 && (fds[1].revents & POLLIN)) {
      struct input_event ev;
      while (libevdev_next_event(mouseDev_, LIBEVDEV_READ_FLAG_NORMAL, &ev) ==
             LIBEVDEV_READ_STATUS_SUCCESS) {
        // Log only non-motion mouse events to avoid flooding
        if (ev.type != EV_REL) {
          logToFile("MSE Event: type=" + std::to_string(ev.type) +
                        " code=" + std::to_string(ev.code) +
                        " value=" + std::to_string(ev.value),
                    LOG_INPUT);
        }
        processEvent(ev, false);
      }
    }
  }
}

void InputMapper::emit(uint16_t type, uint16_t code, int32_t value) {
  libevdev_uinput_write_event(uinputDev_, type, code, value);
  libevdev_uinput_write_event(uinputDev_, EV_SYN, SYN_REPORT, 0);
}

void InputMapper::emitSequence(
    const std::vector<std::pair<uint16_t, int32_t>> &sequence) {
  for (const auto &p : sequence) {
    libevdev_uinput_write_event(uinputDev_, EV_KEY, p.first, p.second);
  }
  libevdev_uinput_write_event(uinputDev_, EV_SYN, SYN_REPORT, 0);
}

void InputMapper::setContext(const std::string &appName,
                             const std::string &url) {
  std::lock_guard<std::mutex> lock(contextMutex_);
  activeApp_ = appName;
  activeUrl_ = url;
  logToFile("Context updated: App=[" + activeApp_ + "] URL=[" + activeUrl_ +
                "]",
            LOG_WINDOW);
}

void InputMapper::processEvent(struct input_event &ev, bool isKeyboard) {
  // Update LeftCtrl state for macros (tracked outside Chrome block)
  if (isKeyboard && ev.type == EV_KEY && ev.code == KEY_LEFTCTRL) {
    ctrlDown_ = (ev.value != 0);
  }

  // Debug: Log all Ctrl+V combinations to see current context
  if (isKeyboard && ev.type == EV_KEY && ev.code == KEY_V && ctrlDown_ &&
      ev.value == 1) {
    std::string tmpApp, tmpUrl;
    {
      std::lock_guard<std::mutex> lock(contextMutex_);
      tmpApp = activeApp_;
      tmpUrl = activeUrl_;
    }
    logToFile("Ctrl+V detected. State: App=[" + tmpApp + "] URL=[" + tmpUrl +
                  "]",
              LOG_INPUT);
  }

  if (isKeyboard && ctrlDown_ && ev.type == EV_KEY && ev.code == KEY_1 &&
      ev.value == 1) {
    std::string logPath;
    for (const auto &f : files.files) {
      if (f.name == "combined.log") {
        logPath = f.fullPath();
        break;
      }
    }
    if (logPath.empty()) {
      logPath = directories.data + "combined.log";
    }
    std::ofstream sf(logPath, std::ios::app);
    if (sf.is_open()) {
      sf << "Sanity check: LeftCtrl + 1 pressed at event time "
         << ev.input_event_sec << "." << ev.input_event_usec << std::endl;
      sf.close();
    }
    logToFile("Sanity check: LeftCtrl + 1 detected", LOG_CORE);
  }

  if (!isKeyboard && ev.type == EV_KEY && ev.code == BTN_FORWARD) {
    emit(EV_KEY, KEY_ENTER, ev.value);
    return;
  }

  if (isKeyboard && ev.type == EV_KEY) {
    if (ev.code == KEY_LEFTCTRL) {
      if (ev.value == 1) {
        if (gToggleState_ == 1) {
          gToggleState_ = 2;
          logToFile("G-Key State: 1 -> 2 (Ctrl)", LOG_INPUT);
        } else if (gToggleState_ == 3) {
          gToggleState_ = 4;
          logToFile("G-Key State: 3 -> 4 (Ctrl)", LOG_INPUT);
        } else {
          gToggleState_ = 1;
        }
      }
    } else if (ev.code == KEY_LEFTSHIFT) {
      if (ev.value == 1) {
        if (gToggleState_ == 2) {
          gToggleState_ = 3;
          logToFile("G-Key State: 2 -> 3 (Shift)", LOG_INPUT);
        } else if (gToggleState_ == 4) {
          gToggleState_ = 5;
          logToFile("G-Key State: 4 -> 5 (Shift)", LOG_INPUT);
        } else {
          gToggleState_ = 1;
        }
      }
    } else if (gToggleState_ == 5) {
      if (ev.code >= KEY_1 && ev.code <= KEY_6) {
        if (ev.code == KEY_1) {
          std::string currentApp;
          {
            std::lock_guard<std::mutex> lock(contextMutex_);
            currentApp = activeApp_;
          }
          if (currentApp == wmClassTerminal) {
            logToFile(
                "Triggering G1 SIGINT macro (Ctrl+Alt+C) for Gnome Terminal",
                LOG_AUTOMATION);
            emit(EV_KEY, KEY_LEFTCTRL, 1);
            emit(EV_KEY, KEY_LEFTALT, 1);
            emit(EV_KEY, KEY_C, 1);
            emit(EV_KEY, KEY_C, 0);
            emit(EV_KEY, KEY_LEFTALT, 0);
            emit(EV_KEY, KEY_LEFTCTRL, 0);
            gToggleState_ = 1;
            return;
          }
        }
        logToFile("G" + std::to_string(ev.code - KEY_1 + 1) + " pressed",
                  LOG_INPUT);
        gToggleState_ = 1;
        return;
      }
      logToFile("G-Key State: Reset due to unexpected key code: " +
                    std::to_string(ev.code),
                LOG_INPUT);
      gToggleState_ = 1;
    } else {
      if (gToggleState_ != 1) {
        // Just log if we are resetting a partial state
        logToFile("G-Key State: Reset from " + std::to_string(gToggleState_) +
                      " due to key code: " + std::to_string(ev.code),
                  LOG_INPUT);
      }
      gToggleState_ = 1;
    }
  }

  // 4. Chrome-specific Ctrl+V macro (ChatGPT only)
  std::string currentApp, currentUrl;
  {
    std::lock_guard<std::mutex> lock(contextMutex_);
    currentApp = activeApp_;
    currentUrl = activeUrl_;
  }

  if (isKeyboard && currentApp == wmClassChrome && ev.type == EV_KEY) {
    if (ev.code == KEY_V && ctrlDown_) {
      // Logic for KEY_V press (value == 1)
      if (ev.value == 1) {
        // Fetch fresh URL ON DEMAND to handle tab switches
        std::string freshUrl = getChromeTabUrl();
        if (!freshUrl.empty()) {
          currentUrl = freshUrl;
          logToFile("On-demand context update: URL=[" + currentUrl + "]",
                    LOG_WINDOW);
          // Update the context via mutex so subsequent logs/logic are correct
          // (Optional, but good for consistency)
          std::lock_guard<std::mutex> lock(contextMutex_);
          activeUrl_ = currentUrl;
        }

        // Relaxed URL check (case-insensitive and not necessarily at start)
        if (currentUrl.find("chatgpt.com") != std::string::npos) {
          logToFile("Triggering ChatGPT Ctrl+V macro. URL: " + currentUrl,
                    LOG_AUTOMATION);
          emit(EV_KEY, KEY_LEFTCTRL, 1);
          emit(EV_KEY, KEY_LEFTCTRL, 0);
          emit(EV_KEY, KEY_H, 1);
          emit(EV_KEY, KEY_H, 0);
          emit(EV_KEY, KEY_I, 1);
          emit(EV_KEY, KEY_I, 0);
          emit(EV_KEY, KEY_BACKSPACE, 1);
          emit(EV_KEY, KEY_BACKSPACE, 0);
          emit(EV_KEY, KEY_BACKSPACE, 1);
          emit(EV_KEY, KEY_BACKSPACE, 0);
          emit(EV_KEY, KEY_LEFTCTRL, 1);
          emit(EV_KEY, KEY_V, 1);
          emit(EV_KEY, KEY_V, 0);
          emit(EV_KEY, KEY_LEFTCTRL, 0);
          return; // Swallowed for ChatGPT
        } else {
          logToFile("Ctrl+V in Chrome (NOT ChatGPT). URL: [" + currentUrl + "]",
                    LOG_AUTOMATION);
          // Fall through to default emit for regular tabs
        }
      } else {
        // For key release (value == 0) or repeat (value == 2)
        // Check if we should swallow it based on the URL we decided on press?
        // Actually, if we swallowed the press, we must swallow the release.
        // But here we are fetching URL on press.
        // A simple heuristic: if it *looks* like ChatGPT, we swallow.
        // Note: activeUrl_ might have been updated by the press.
        // Re-reading is safe.
        if (currentUrl.find("chatgpt.com") != std::string::npos) {
          return; // Swallow release/repeat for ChatGPT
        }
      }
    }
  }

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

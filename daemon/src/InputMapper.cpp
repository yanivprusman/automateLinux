#include "InputMapper.h"
#include "Constants.h"
#include "Globals.h"
#include "Utils.h"
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
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
    logToFile("Failed to open keyboard device: " + keyboardPath_);
    return false;
  }

  if (libevdev_new_from_fd(keyboardFd_, &keyboardDev_) < 0) {
    logToFile("Failed to initialize libevdev for keyboard");
    return false;
  }

  if (libevdev_grab(keyboardDev_, LIBEVDEV_GRAB) < 0) {
    std::cerr << "CRITICAL: Failed to grab keyboard: " << strerror(errno)
              << std::endl;
    logToFile("Failed to grab keyboard");
    return false;
  }
  std::cerr << "InputMapper: Keyboard grabbed successfully" << std::endl;

  if (!mousePath_.empty()) {
    mouseFd_ = open(mousePath_.c_str(), O_RDONLY | O_NONBLOCK);
    if (mouseFd_ >= 0) {
      if (libevdev_new_from_fd(mouseFd_, &mouseDev_) < 0) {
        logToFile("Failed to initialize libevdev for mouse");
      } else {
        if (libevdev_grab(mouseDev_, LIBEVDEV_GRAB) < 0) {
          logToFile("Failed to grab mouse");
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
    logToFile("Failed to create uinput device: " + std::string(strerror(-rc)));
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
                  " value=" + std::to_string(ev.value));
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
                    " value=" + std::to_string(ev.value));
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

void InputMapper::processEvent(struct input_event &ev, bool isKeyboard) {
  // Update LeftCtrl state for macros (tracked outside Chrome block)
  if (isKeyboard && ev.type == EV_KEY && ev.code == KEY_LEFTCTRL) {
    ctrlDown_ = (ev.value != 0);
  }

  // 0. Sanity Check Macro (LeftCtrl + 1)
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
    logToFile("Sanity check: LeftCtrl + 1 detected");
  }

  // Porting logic from corsairKeyBoardLogiMouseAll.sh

  // 1. Mouse forward to Enter
  if (!isKeyboard && ev.type == EV_KEY && ev.code == BTN_FORWARD) {
    emit(EV_KEY, KEY_ENTER, ev.value);
    return;
  }

  // 2. G-key detection (GToggle)
  // --hook key:leftctrl@gProgress1 toggle=GToggle:2
  // --hook @gProgress2 toggle=GToggle:1 ... etc
  if (isKeyboard && ev.type == EV_KEY) {
    if (ev.code == KEY_LEFTCTRL) {
      if (ev.value == 1) { // Down
        if (gToggleState_ == 1)
          gToggleState_ = 2;
        else if (gToggleState_ == 3)
          gToggleState_ = 4;
        else
          gToggleState_ = 1;
      }
    } else if (ev.code == KEY_LEFTSHIFT) {
      if (ev.value == 1) { // Down
        if (gToggleState_ == 2)
          gToggleState_ = 3;
        else if (gToggleState_ == 4)
          gToggleState_ = 5;
        else
          gToggleState_ = 1;
      }
    } else if (gToggleState_ == 5) {
      // Check for G1-G6 keys (which are sent as number keys 1-6 in this state)
      if (ev.code >= KEY_1 && ev.code <= KEY_6) {
        // evsieve script says: echo G1, but doesn't map them to anything else
        // in All.sh? Wait, All.sh has "--hook key:1@gProgress5 key:1@keyboard
        // exec-shell='echo G1' breaks-on=key::1 sequential --withhold" This
        // means it blocks the key:1 and executes shell.
        logToFile("G" + std::to_string(ev.code - KEY_1 + 1) + " pressed");
        gToggleState_ = 1;
        return; // Block the key
      }
      gToggleState_ = 1;
    } else {
      // Reset state if any other key is pressed
      gToggleState_ = 1;
    }
  }

  // 3. App detection (keyboardToggle)
  // --hook msc:scan:$codeForAppCodes toggle=appCodesToggle
  if (isKeyboard && ev.type == EV_MSC && ev.code == MSC_SCAN) {
    if (ev.value == atoi(VALUE_FOR_APP_CODES)) {
      appCodesToggle_ = true;
      appCodesCount_ = 0;
      return;
    }
    if (appCodesToggle_) {
      appCodesCount_++;
      if (appCodesCount_ == 3) { // After 3 intermediate events
        if (ev.value == atoi(VALUE_FOR_CODE))
          keyboardToggle_ = 1;
        else if (ev.value == atoi(VALUE_FOR_GNOME_TERMINAL))
          keyboardToggle_ = 2;
        else if (ev.value == atoi(VALUE_FOR_GOOGLE_CHROME))
          keyboardToggle_ = 3;
        else if (ev.value == atoi(VALUE_FOR_DEFAULT))
          keyboardToggle_ = 0;
        appCodesToggle_ = false;
      }
      return;
    }
  }

  // 4. Chrome-specific Ctrl+V macro
  if (isKeyboard && keyboardToggle_ == 3 && ev.type == EV_KEY) {
    static bool ctrlDown = false;
    if (ev.code == KEY_LEFTCTRL)
      ctrlDown = (ev.value != 0);

    if (ev.code == KEY_V && ctrlDown) {
      if (ev.value == 1) { // Down
        // Complex sequence from script:
        // key:leftctrl:1@$codeForCntrlV key:leftctrl:0@$codeForCntrlV key:h:1
        // key:h:0 key:i:1 key:i:0 key:backspace:1 key:backspace:0
        // key:backspace:1 key:backspace:0 key:leftctrl:1@$codeForCntrlV
        // key:v:1@keyboard$codeForGoogleChrome key:leftctrl:0@$codeForCntrlV
        // key:v:0@keyboard$codeForGoogleChrome

        // Simplified version for now (just hi + backspaces + ctrl+v)
        // Note: @$codeForCntrlV in evsieve is just a tag, we don't need to do
        // anything special here unless it changes behavior.
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
        return;
      }
      return; // Block normal V if Ctrl is down
    }
  }

  // Default: pass through
  int rc = libevdev_uinput_write_event(uinputDev_, ev.type, ev.code, ev.value);
  if (rc < 0) {
    char errBuf[128];
    int len = snprintf(errBuf, sizeof(errBuf),
                       "Failed to write to uinput: %s (type=%d, code=%d)\n",
                       strerror(-rc), ev.type, ev.code);
    write(STDERR_FILENO, errBuf, len);
    logToFile("Failed to write to uinput: " + std::string(strerror(-rc)));
  }
}

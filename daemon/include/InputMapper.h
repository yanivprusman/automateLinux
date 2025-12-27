#ifndef INPUT_MAPPER_H
#define INPUT_MAPPER_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class InputMapper {
public:
  InputMapper();
  ~InputMapper();

  bool start(const std::string &keyboardPath, const std::string &mousePath);
  void onFocusAck();
  void stop();
  void setContext(const std::string &appName, const std::string &url = "",
                  const std::string &title = "");
  bool isRunning() const { return running_; }

private:
  void loop();
  bool setupDevices();
  bool setupUinput();
  void processEvent(struct input_event &ev, bool isKeyboard);
  void emit(uint16_t type, uint16_t code, int32_t value);
  void emitSequence(const std::vector<std::pair<uint16_t, int32_t>> &sequence);

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

  // --- State Machine for evsieve logic ---

  // G-key sequence detection (corresponds to GToggle in evsieve script)
  // Sequence: Ctrl(down) -> Shift(down) -> Ctrl(down) -> Shift(down) ->
  // Key(number)
  int gToggleState_ = 1; // 1-5

  // Window/App detection context
  std::string activeApp_;
  std::string activeUrl_;
  std::string activeTitle_;
  std::mutex contextMutex_;
};

#endif // INPUT_MAPPER_H

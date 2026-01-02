#include <X11/Xlib.h>
#include <iostream>
int main() {
  Display *display = XOpenDisplay(nullptr);
  if (!display) {
    std::cerr << "XOpenDisplay failed" << std::endl;
    return 1;
  }
  std::cout << "XOpenDisplay succeeded: " << DisplayString(display)
            << std::endl;
  XCloseDisplay(display);
  return 0;
}

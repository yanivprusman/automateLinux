#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>
int main() {
  Display *display = XOpenDisplay(nullptr);
  if (!display)
    return 1;
  Window focusWin;
  int revertTo;
  XGetInputFocus(display, &focusWin, &revertTo);
  std::cout << "Focus: " << focusWin << std::endl;
  Window current = focusWin;
  while (current != None) {
    char *name = nullptr;
    XFetchName(display, current, &name);
    XClassHint ch;
    bool hasClass = XGetClassHint(display, current, &ch);
    std::cout << "Window " << current << ": Name=[" << (name ? name : "")
              << "] Class=[" << (hasClass ? ch.res_class : "") << "] Instance=["
              << (hasClass ? ch.res_name : "") << "]" << std::endl;
    if (name)
      XFree(name);
    if (hasClass) {
      if (ch.res_class)
        XFree(ch.res_class);
      if (ch.res_name)
        XFree(ch.res_name);
    }

    Window root, parent, *children;
    unsigned int n;
    if (!XQueryTree(display, current, &root, &parent, &children, &n))
      break;
    if (children)
      XFree(children);
    if (current == root)
      break;
    current = parent;
  }
  XCloseDisplay(display);
  return 0;
}

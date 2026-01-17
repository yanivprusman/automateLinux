#ifndef KEY_NAMES_H
#define KEY_NAMES_H

#include <linux/input.h>
#include <map>
#include <string>

// This mapping is shared between the daemon and sendKeys utility
// to ensure consistency in key names.

inline const std::map<std::string, int> KEY_NAME_TO_CODE = {
    {"keyA", KEY_A},
    {"keyB", KEY_B},
    {"keyC", KEY_C},
    {"keyD", KEY_D},
    {"keyE", KEY_E},
    {"keyF", KEY_F},
    {"keyG", KEY_G},
    {"keyH", KEY_H},
    {"keyI", KEY_I},
    {"keyJ", KEY_J},
    {"keyK", KEY_K},
    {"keyL", KEY_L},
    {"keyM", KEY_M},
    {"keyN", KEY_N},
    {"keyO", KEY_O},
    {"keyP", KEY_P},
    {"keyQ", KEY_Q},
    {"keyR", KEY_R},
    {"keyS", KEY_S},
    {"keyT", KEY_T},
    {"keyU", KEY_U},
    {"keyV", KEY_V},
    {"keyW", KEY_W},
    {"keyX", KEY_X},
    {"keyY", KEY_Y},
    {"keyZ", KEY_Z},
    /* Symbol keys */
    {"period", KEY_DOT},
    {"dot", KEY_DOT},
    {"slash", KEY_SLASH},
    {"minus", KEY_MINUS},
    {"dash", KEY_MINUS},
    {"space", KEY_SPACE},
    {"comma", KEY_COMMA},
    {"equals", KEY_EQUAL},
    {"equal", KEY_EQUAL},
    {"backspace", KEY_BACKSPACE},
    {"semicolon", KEY_SEMICOLON},
    {"apostrophe", KEY_APOSTROPHE},
    {"quote", KEY_APOSTROPHE},
    {"backslash", KEY_BACKSLASH},
    {"bracket_left", KEY_LEFTBRACE},
    {"leftbracket", KEY_LEFTBRACE},
    {"bracket_right", KEY_RIGHTBRACE},
    {"rightbracket", KEY_RIGHTBRACE},
    {"backtick", KEY_GRAVE},
    {"grave", KEY_GRAVE},
    {"enter", KEY_ENTER},
    {"keyShift", KEY_LEFTSHIFT},
    {"keyCtrl", KEY_LEFTCTRL},
    {"keyAlt", KEY_LEFTALT},
    {"keyMeta", KEY_LEFTMETA},
    {"keyTab", KEY_TAB},
    {"keyCaps", KEY_CAPSLOCK},
    {"keyEsc", KEY_ESC},
    {"keyF1", KEY_F1},
    {"keyF2", KEY_F2},
    {"keyF3", KEY_F3},
    {"keyF4", KEY_F4},
    {"keyF5", KEY_F5},
    {"keyF6", KEY_F6},
    {"keyF7", KEY_F7},
    {"keyF8", KEY_F8},
    {"keyF9", KEY_F9},
    {"keyF10", KEY_F10},
    {"keyF11", KEY_F11},
    {"keyF12", KEY_F12},
    {"keyUp", KEY_UP},
    {"keyDown", KEY_DOWN},
    {"keyLeft", KEY_LEFT},
    {"keyRight", KEY_RIGHT},
    {"keyPageUp", KEY_PAGEUP},
    {"keyPageDown", KEY_PAGEDOWN},
    {"keyHome", KEY_HOME},
    {"keyEnd", KEY_END},
    {"keyInsert", KEY_INSERT},
    {"keyDelete", KEY_DELETE},
    {"numlock", KEY_NUMLOCK},
    {"enter", KEY_ENTER},
    {"syn", 0}, // Special case
};

inline const std::map<std::string, int> APP_NAME_TO_CODE = {
    {"code", 102},
    {"gnome-terminal-server", 103},
    {"google-chrome", 104},
    {"DefaultKeyboard", 101},
};

const int KEY_CODE_FOR_APP_SWITCH = 100;

#endif // KEY_NAMES_H

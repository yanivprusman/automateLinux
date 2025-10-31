# Terminal Control Library

A C++ library for advanced terminal manipulation and control. This library provides low-level access to terminal features while maintaining a clean, modern C++ interface.

## Features

- Raw terminal mode handling
- Cursor control and positioning
- Screen buffer management
- Color and text attribute support
- Terminal regions with scrolling
- Event handling for keyboard input
- Cross-platform terminal manipulation (Unix-like systems)

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Example Usage

```cpp
#include <terminal.hpp>

int main() {
    termcontrol::Terminal term;
    
    // Initialize terminal
    term.init();
    term.setRawMode(true);
    
    // Clear screen
    term.clear();
    
    // Move cursor and print
    term.moveCursor(0, 0);
    term.setBold(true);
    std::cout << "Hello, Terminal!" << std::endl;
    
    // Wait for key
    term.readInput([](char c) { return; });
    
    // Cleanup
    term.setRawMode(false);
    return 0;
}
```

## Components

- `Terminal`: Main class for terminal control
- `Buffer`: Screen buffer management
- `Region`: Terminal region management
- Events: Input event handling system

## License

MIT License
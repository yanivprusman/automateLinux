# KEY_ENTER (Code 28) vs KEY_KPENTER (Code 96) Analysis

## CRITICAL FINDING: THE BUG

### Root Cause Location
**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp#L432-L450)  
**Lines:** 432-450

### The Problem

The issue is in the `processEvent` function's control flow. The function has a structure like this:

```cpp
void InputMapper::processEvent(struct input_event &ev, bool isKeyboard, bool skipMacros) {
  // ... combo matching logic (lines 285-428)
  
  // *** CRITICAL: If a combo matches and eventConsumed=true, function returns! ***
  if (eventConsumed) {
    // ... handle suppression and flushing ...
    return;  // <-- EARLY EXIT HERE
  }
  
  // ... more logic (lines 430-432)
  
  // *** THIS BLOCK ONLY RUNS IF NO COMBO WAS MATCHED ***
  if (ev.type == EV_KEY && (ev.code == KEY_ENTER || ev.code == KEY_KPENTER)) {
    logToFile("Processing ENTER key (code " + std::to_string(ev.code) +
                  ", value " + std::to_string(ev.value) + ") -> uinput",
              LOG_INPUT);
  }

  // *** THIS IS THE FINAL EMIT THAT BYPASSES ALL COMBO LOGIC ***
  std::lock_guard<std::mutex> lock(uinputMutex_);
  int rc = libevdev_uinput_write_event(uinputDev_, ev.type, ev.code, ev.value);
  // ...
}
```

### The Specific Issue with KEY_ENTER

**KEY_ENTER (code 28)** is used in the default macro:

**File:** [daemon/src/MacroConfig.cpp](daemon/src/MacroConfig.cpp#L6-L13)  
**Lines:** 10-11

```cpp
defaultMacros.push_back(
    KeyAction{KeyTrigger{{{BTN_FORWARD, 1, false}}},
              {{KEY_ENTER, 1}, {KEY_ENTER, 0}},  // <-- Action emits KEY_ENTER
              "Triggering mouse forward button (press) → Enter",
              nullptr});
```

**What happens:**
1. When **BTN_FORWARD** (mouse forward button) is pressed, it matches a combo
2. `eventConsumed = true`
3. `executeKeyAction()` is called, which calls `emitSequence()`
4. `emitSequence()` directly calls `libevdev_uinput_write_event()` for KEY_ENTER press/release
5. The function **returns early** (line 402)
6. **KEY_ENTER is never grabbed from the input device** because the original KEY_ENTER key events are not being processed through the normal event handling

**Why KEY_KPENTER (96) works:**
- KEY_KPENTER is NOT in any macro trigger or action sequence
- When KEY_KPENTER arrives at the `processEvent()` function, it doesn't match any combo
- `eventConsumed` remains `false`
- The function **does NOT return early**
- It falls through to the final `libevdev_uinput_write_event()` call at line 441
- KEY_KPENTER is successfully passed to uinput

### The Dual Path Problem

The `processEvent()` function has **two different paths** for sending keys to uinput:

1. **Path 1 (via combo actions)** - `emit()` and `emitSequence()` functions
   - Called when a macro is triggered
   - Used for injecting KEY_ENTER from macro
   - Returns early, so primary passthrough logic is skipped

2. **Path 2 (final fallback)** - Direct `libevdev_uinput_write_event()` at line 441
   - Called for all other keys
   - KEY_KPENTER uses this path (works fine)
   - KEY_ENTER bypasses this due to early return

### Why KEY_ENTER is Blocked/Non-Functional

When you try to press the **main keyboard Enter key (code 28)**:
1. It arrives at `processEvent()`
2. It attempts to match combos but doesn't match any trigger
3. `eventConsumed = false`
4. The function proceeds to the final `libevdev_uinput_write_event()` call
5. **BUT**: The keyboard device is **grabbed** by the daemon

**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp#L82-L93)  
**Lines:** 82-93 (in `setupDevices()`):

```cpp
if (libevdev_grab(keyboardDev_, LIBEVDEV_GRAB) < 0) {
  std::cerr << "CRITICAL: Failed to grab keyboard: " << strerror(errno)
            << std::endl;
  logToFile("Failed to grab keyboard", LOG_CORE);
  return false;
}
logToFile("InputMapper: Keyboard grabbed successfully", LOG_CORE);
```

When the keyboard is grabbed, the original KEY_ENTER events from the physical keyboard are **intercepted** and the input is completely captured by the daemon. If the daemon doesn't re-emit them through uinput, they are **lost**.

## Architecture Issue Summary

| Aspect | KEY_ENTER (28) | KEY_KPENTER (96) |
|--------|---|---|
| **Used in macro?** | YES - as action output | NO |
| **Code path when injected via macro** | Through `emitSequence()` (Path 1) | N/A |
| **Code path when pressed physically** | Early return prevents proper handling | Falls through to final `libevdev_uinput_write_event()` (Path 2) |
| **Keyboard grab impact** | Physical KEY_ENTER blocked and lost | Works fine |
| **Result** | Does NOT work when pressed on physical keyboard | Works fine |

## Diagram of Control Flow

```
processEvent(KEY_ENTER) [physically pressed]
    ↓
[Combo matching: no match found]
    ↓
eventConsumed = false
    ↓
Continue to line 432
    ↓
Log message at line 434-436
    ↓
Finally emit at line 441 ✓ (WORKS - physical KEY_ENTER passes through)
```

```
processEvent(BTN_FORWARD) [physically pressed]
    ↓
[Combo matching: MATCHES macro]
    ↓
eventConsumed = true
    ↓
executeKeyAction() → emitSequence()
    ↓
Write KEY_ENTER, 1 to uinput ✓
Write KEY_ENTER, 0 to uinput ✓
    ↓
return at line 402 (EARLY EXIT)
    ↓
[Function exits - never reaches line 441]
```

**BUT** when you physically press KEY_ENTER on keyboard while it's grabbed:
```
Physical KEY_ENTER press event arrives
    ↓
Daemon grabs keyboard (line 87)
    ↓
Physical KEY_ENTER is intercepted
    ↓
processEvent() is called
    ↓
No combo matches
    ↓
Falls through to emit at line 441
    ↓
Gets re-emitted via uinput ✓ (SHOULD WORK)
    ↓
BUT if there's any suppression logic or pending event blocking...
    ↓
KEY_ENTER release event may be stuck in suppression state!
```

## Potential Suppressions for KEY_ENTER

Looking at the combo matching and suppression logic, there are complex interactions:

**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp#L301-L355)

The combo logic tracks:
- `shouldSuppressThisSpecificEvent` (line 301)
- `pendingEvents_` queue (for suppressed keys)
- `anyComboInProgress` state

**Risk:** If KEY_ENTER physically pressed while a combo is in progress that doesn't explicitly suppress KEY_ENTER, the release event may be queued and not flushed properly.

## The Real Blocker: Keyboard Grab + Suppression Logic

The combination of:
1. **Keyboard grab** at device level (prevents system from seeing physical KEY_ENTER)
2. **Combo suppression logic** that may incorrectly queue KEY_ENTER events
3. **Dual code paths** (emitSequence vs final emit) that aren't properly synchronized

...creates a situation where physically-pressed KEY_ENTER cannot escape the daemon's processing.

## Files Involved

1. **[daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp)** - Main event processing
   - Line 87: Keyboard grab
   - Line 285-428: Combo matching and suppression logic
   - Line 402: Early return when combo matches
   - Line 434-441: ENTER key logging and final emit

2. **[daemon/src/MacroConfig.cpp](daemon/src/MacroConfig.cpp)** - Macro definitions
   - Line 10-11: DEFAULT MACRO uses KEY_ENTER as action output

3. **[daemon/include/InputMapper.h](daemon/include/InputMapper.h)** - Class definition
   - Line 35: suppressedKeys tracking
   - Line 39-46: KeyTrigger definition

## Verification Points

- ✓ KEY_ENTER (28) is explicitly handled in logging (line 434-436)
- ✓ KEY_KPENTER (96) is explicitly handled in logging (same lines 434-436)
- ✓ Both should pass through the final emit (line 441)
- ✓ Both should work identically based on code review
- **✗ But only KEY_KPENTER actually works in practice**

This suggests the issue is in the **suppression/queueing logic** (lines 301-430) blocking KEY_ENTER specifically.

## Next Step: Find Where KEY_ENTER is Getting Suppressed

The logs should reveal:
1. Does "Processing ENTER key" get logged for physical KEY_ENTER presses?
2. Does "Flushing pending events" or "Flushing remaining" appear for KEY_ENTER?
3. Is KEY_ENTER stuck in the `pendingEvents_` queue?
4. Does "Combo broken" appear, indicating KEY_ENTER breaks an in-progress combo?

If physical KEY_ENTER is not reaching the "Processing ENTER key" log, it's being suppressed or not read from the grabbed device at all.

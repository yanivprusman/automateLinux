# Exact Code Locations: KEY_ENTER vs KEY_KPENTER Handling

## Summary Table

| Item | Location | Code |
|------|----------|------|
| KEY_ENTER in macro action | [daemon/src/MacroConfig.cpp:10](daemon/src/MacroConfig.cpp#L10) | `{{KEY_ENTER, 1}, {KEY_ENTER, 0}}` |
| KEY_ENTER logging | [daemon/src/InputMapper.cpp:434](daemon/src/InputMapper.cpp#L434) | Both codes logged here |
| KEY_KPENTER logging | [daemon/src/InputMapper.cpp:434](daemon/src/InputMapper.cpp#L434) | Both codes logged here |
| Final emit function | [daemon/src/InputMapper.cpp:441](daemon/src/InputMapper.cpp#L441) | `libevdev_uinput_write_event(uinputDev_, ev.type, ev.code, ev.value)` |
| Combo matching start | [daemon/src/InputMapper.cpp:285](daemon/src/InputMapper.cpp#L285) | `if (!skipMacros && ev.type == EV_KEY)` |
| Early return (key blocker) | [daemon/src/InputMapper.cpp:402](daemon/src/InputMapper.cpp#L402) | `return;` when eventConsumed |
| Suppression queue logic | [daemon/src/InputMapper.cpp:410-428](daemon/src/InputMapper.cpp#L410-L428) | Queue management |
| Keyboard grab | [daemon/src/InputMapper.cpp:87](daemon/src/InputMapper.cpp#L87) | `libevdev_grab(keyboardDev_, LIBEVDEV_GRAB)` |

---

## 1. KEY_ENTER MACRO DEFINITION

**File:** [daemon/src/MacroConfig.cpp](daemon/src/MacroConfig.cpp)  
**Lines:** 10-13

```cpp
defaultMacros.push_back(
    KeyAction{KeyTrigger{{{BTN_FORWARD, 1, false}}},
              {{KEY_ENTER, 1}, {KEY_ENTER, 0}},  // ← KEY_ENTER emitted here
              "Triggering mouse forward button (press) → Enter",
              nullptr});
```

**Analysis:**
- When `BTN_FORWARD` (mouse forward button) is pressed, the daemon **injects** KEY_ENTER press + release
- This is sent via `emitSequence()` which uses Path 1 (direct `libevdev_uinput_write_event`)

---

## 2. KEYBOARD GRAB (The Interception Point)

**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp)  
**Lines:** 82-93 (in `setupDevices()` function)

```cpp
if (libevdev_grab(keyboardDev_, LIBEVDEV_GRAB) < 0) {
  std::cerr << "CRITICAL: Failed to grab keyboard: " << strerror(errno)
            << std::endl;
  logToFile("Failed to grab keyboard", LOG_CORE);
  return false;
}
logToFile("InputMapper: Keyboard grabbed successfully", LOG_CORE);
```

**Impact:**
- All physical keyboard input (including KEY_ENTER) is intercepted
- The kernel does NOT send these events to other applications
- The daemon MUST re-emit them via uinput or they're lost
- This is the **critical bottleneck** for physical KEY_ENTER

---

## 3. COMBO MATCHING LOGIC - THE CONTROL FLOW

**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp)  
**Lines:** 285-405

### 3a. Combo Matching Entry Point

**Line 285:**
```cpp
if (!skipMacros && ev.type == EV_KEY) { // Any key event
  AppType currentApp;
  {
    std::lock_guard<std::mutex> lock(contextMutex_);
    currentApp = activeApp_;
  }

  auto appIt = appMacros_.find(currentApp);
  if (appIt != appMacros_.end()) {
    // ... combo matching logic ...
  }
}
```

### 3b. Per-Combo Matching Loop

**Lines:** 303-367

```cpp
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
      // MATCH! Advance state
      state.nextKeyIndex++;
      eventConsumed = true; // ← KEY FLAG SET
      
      if (shouldSuppress) {
        shouldSuppressThisSpecificEvent = true;
        state.suppressedKeys.push_back({ev.code, (uint8_t)ev.value});
      }
      // ... combo completion check ...
    }
  }
}
```

**Critical Variables:**
- `eventConsumed` (line 301): **Set to true if ANY combo step matched**
- `shouldSuppressThisSpecificEvent` (line 301): Set if this key should be held back
- `pendingEvents_` (line 326): Queue for held-back keys

### 3c. Early Return (KEY BLOCKER)

**Lines:** 381-405

```cpp
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

  if (completedSuppressing && !anyComboInProgress) {
    // If we just finished a suppressing combo and nothing else is
    // in progress, flush the queue.
    std::lock_guard<std::mutex> lock(pendingEventsMutex_);
    if (!pendingEvents_.empty()) {
      logToFile("Flushing remaining after complete", LOG_INPUT);
      for (const auto &pe : pendingEvents_) {
        emit(pe.type, pe.code, pe.value);
      }
      pendingEvents_.clear();
    }
  }
  return;  // ← **EARLY EXIT** - Never reaches final emit!
}
```

**Critical Issue:**
- If `eventConsumed = true`, the function **returns early**
- The final emit at line 441 is **NEVER REACHED**
- For unmatched keys, the function continues (next section)

### 3d. Queue Management for Pending Events

**Lines:** 408-428**

```cpp
// If we are currently in a suppressed state (holding a trigger key), we
// must queue even unmatched keys to preserve order.
{
  std::lock_guard<std::mutex> lock(pendingEventsMutex_);
  currentlySuppressing = !pendingEvents_.empty();
}

if (currentlySuppressing && anyComboInProgress) {
  // Still matching some other potential combo, keep queueing
  std::lock_guard<std::mutex> lock(pendingEventsMutex_);
  pendingEvents_.push_back(
      {(uint16_t)ev.type, (uint16_t)ev.code, (int32_t)ev.value});
  return;  // ← **EARLY EXIT** - Queue and don't emit!
} else {
  // Not matching anything anymore, or we weren't suppressing. Flush.
  std::lock_guard<std::mutex> lock(pendingEventsMutex_);
  if (!pendingEvents_.empty()) {
    logToFile("Flushing pending events (mismatch/broken combo)",
              LOG_INPUT);
    for (const auto &pe : pendingEvents_) {
      emit(pe.type, pe.code, pe.value);
    }
    pendingEvents_.clear();
  }
}
```

**Critical Issue #2:**
- **Line 417-422:** If `currentlySuppressing && anyComboInProgress`, the event is **queued and function returns**
- Physical KEY_ENTER could be queued here and **never flushed** if combo never completes

---

## 4. THE DUAL EMIT PATHS

### Path 1: `emit()` function (for combo actions)

**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp)  
**Lines:** 214-220

```cpp
void InputMapper::emit(uint16_t type, uint16_t code, int32_t value) {
  if (!uinputDev_)
    return;
  std::lock_guard<std::mutex> lock(uinputMutex_);
  libevdev_uinput_write_event(uinputDev_, type, code, value);
  libevdev_uinput_write_event(uinputDev_, EV_SYN, SYN_REPORT, 0);
}
```

**Used by:**
- Line 388: When event matched combo but NOT suppressed
- Line 398: When flushing completed combo
- Line 426: When flushing pending events

### Path 2: `emitSequence()` function (for macro injection)

**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp)  
**Lines:** 222-231

```cpp
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
```

**Used by:**
- Line 512: When executing macro action (like BTN_FORWARD → KEY_ENTER)

### Path 3: Final Direct Emit (for unmatched events)

**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp)  
**Lines:** 434-451

```cpp
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
```

**Only reached if:**
- Function never returns early (lines 402, 417, 422)
- All combo matching completes or no combos exist

---

## 5. THE CRITICAL DIFFERENCE

### KEY_KPENTER (96) Flow

```
1. Physical KEY_KPENTER press arrives
2. Keyboard grab intercepts it
3. processEvent() called
4. Combo matching: NO MATCH (not in any macro)
5. eventConsumed = false
6. Continue past line 405
7. Queue management: no pending events, continue
8. Continue past line 428
9. Reaches line 434 logging ✓
10. Reaches line 441 final emit ✓
11. KEY_KPENTER sent to uinput ✓
```

### KEY_ENTER (28) Flow When Pressed Physically

```
1. Physical KEY_ENTER press arrives
2. Keyboard grab intercepts it
3. processEvent() called
4. Combo matching: NO MATCH (KEY_ENTER only in macro OUTPUTS)
5. eventConsumed = false
6. Continue past line 405
7. Queue management: CRITICAL QUESTION
   - Is pendingEvents_ non-empty?
   - Is anyComboInProgress true?
   - If BOTH true: Event gets queued at line 420
8. If queued: return at line 422 - NEVER REACHES FINAL EMIT
9. If not queued: Continue to line 434 logging and line 441 emit
```

**Hypothesis:**
Physical KEY_ENTER is being **queued and never flushed** if there's an in-progress combo that doesn't explicitly suppress it.

---

## 6. COMBO COMPLETION AND FLUSH LOGIC

**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp)  
**Lines:** 336-357

```cpp
// Check if combo is complete
if (state.nextKeyIndex == action.trigger.keyCodes.size()) {
  logToFile("COMBO COMPLETE: " + action.logMessage, LOG_AUTOMATION);

  if (action.trigger.hasSuppressedKeys) {
    completedSuppressing = true;
    // Consume trigger from queue (the keys that match the combo)
    std::lock_guard<std::mutex> lock(pendingEventsMutex_);
    // We only want to remove events that we actually suppressed for THIS combo
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
```

**Issue:**
- Only removes **explicitly suppressed keys** from queue
- If KEY_ENTER was queued but NOT in `suppressedKeys`, it may remain in queue indefinitely

---

## 7. PENDING EVENT STRUCTURE

**File:** [daemon/include/InputMapper.h](daemon/include/InputMapper.h)  
**Lines:** 47-56

```cpp
struct PendingEvent {
  uint16_t type;
  uint16_t code;
  int32_t value;
};

// Private members
std::vector<PendingEvent> pendingEvents_;
std::mutex pendingEventsMutex_;
```

**Location in file:** Search for `pendingEvents_` usage

---

## 8. SUPPRESSION FLAG DEFINITION

**File:** [daemon/include/InputMapper.h](daemon/include/InputMapper.h)  
**Lines:** 39-46

```cpp
struct KeyTrigger {
  std::vector<std::tuple<uint16_t, uint8_t, bool>>
      keyCodes; // Sequence of (code, state: 1=press/0=release, suppress: true/false)
  bool hasSuppressedKeys = false; // Cached check for efficiency
};
```

---

## Key Findings Summary

1. **KEY_ENTER** is explicitly named in macro output (MacroConfig.cpp:10)
2. **KEY_ENTER** and **KEY_KPENTER** have identical logging (InputMapper.cpp:434)
3. **KEY_ENTER** and **KEY_KPENTER** should use identical final emit path (InputMapper.cpp:441)
4. **BUT** the suppression/queueing logic (InputMapper.cpp:408-428) can block physical KEY_ENTER
5. **The grab** at line 87 is the enforcement mechanism - blocking re-emission means it's lost forever

## Smoking Gun: The Queue Logic

If physical KEY_ENTER arrives while:
- `pendingEvents_` is non-empty (some keys already queued)
- `anyComboInProgress = true` (some combo hasn't completed or broken)
- KEY_ENTER is NOT explicitly suppressed by that combo

**Then:** KEY_ENTER gets queued at line 420  
**And:** Function returns at line 422, never reaching the final emit  
**Result:** KEY_ENTER remains in queue indefinitely or until combo breaks/completes

This explains why:
- **KEY_KPENTER works:** Not in any combo, so never gets queued
- **KEY_ENTER fails:** Could match various combos' "in progress" state and get queued

The fix would need to ensure KEY_ENTER is either:
1. Excluded from queueing
2. Properly flushed after queue operations
3. Handled specially like other critical keys

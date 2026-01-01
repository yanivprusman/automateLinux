# EXECUTIVE SUMMARY: KEY_ENTER vs KEY_KPENTER Issue

## Problem Statement

- **Working:** Key code 96 (KEY_KPENTER - numeric keypad Enter) works fine when injected via uinput
- **Failing:** Key code 28 (KEY_ENTER - main keyboard Enter) does not work when pressed on physical keyboard
- **Both are logged:** Both key codes appear in the logging code
- **Question:** Why does one work and the other doesn't?

---

## Root Cause: Queue Deadlock

**The main keyboard Enter key (code 28) gets stuck in the pending events queue and is never flushed.**

### Why This Happens

The `processEvent()` function in `InputMapper.cpp` has a queue management system:

1. **When a macro trigger key is pressed:**
   - It's marked as suppressed (should be held back)
   - Added to `pendingEvents_` queue
   - `anyComboInProgress = true`

2. **When physical KEY_ENTER is subsequently pressed:**
   - Doesn't match any combo (it's an OUTPUT, not a TRIGGER)
   - `eventConsumed = false`
   - But enters queue logic because:
     - `currentlySuppressing = true` (pendingEvents_ has trigger key)
     - `anyComboInProgress = true` (combo still in progress)
   - **Gets added to queue at line 420** to preserve key order

3. **When the original combo completes:**
   - Flush logic only removes EXPLICITLY SUPPRESSED keys (line 356)
   - KEY_ENTER was queued but NOT suppressed by that specific combo
   - **Remains in queue indefinitely** ← DEADLOCK

4. **Result:**
   - KEY_ENTER never reaches the final `emit()` at line 441
   - Never sent to uinput
   - Application never receives the key press

---

## Why KEY_KPENTER Works

**KEY_KPENTER avoids the queue system entirely:**

1. When KEY_KPENTER is pressed
2. No combo matches it (not in any trigger or action)
3. If `pendingEvents_` is empty, it doesn't enter queue logic
4. Falls through to final emit at line 441 ✓
5. Gets sent to uinput successfully ✓

**KEY_KPENTER never gets queued because:**
- It's not part of any macro definition
- It never triggers the condition `currentlySuppressing && anyComboInProgress`

---

## Exact Code Locations

### The Bottleneck: Queue Logic (Lines 408-428)

**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp#L408-L428)

```cpp
if (currentlySuppressing && anyComboInProgress) {
  // Still matching some other potential combo, keep queueing
  std::lock_guard<std::mutex> lock(pendingEventsMutex_);
  pendingEvents_.push_back(
      {(uint16_t)ev.type, (uint16_t)ev.code, (int32_t)ev.value});
  return;  // ← EARLY EXIT: Function stops here, never reaches line 441
}
```

**Issue:** 
- Queues ANY event, even unrelated ones
- Assumes all queued events need suppression
- Early return prevents final emit for queued events

### The Incomplete Flush: Combo Completion (Lines 336-357)

**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp#L336-L357)

```cpp
if (state.nextKeyIndex == action.trigger.keyCodes.size()) {
  // Combo complete - flush suppressed keys
  if (action.trigger.hasSuppressedKeys) {
    completedSuppressing = true;
    std::lock_guard<std::mutex> lock(pendingEventsMutex_);
    // Only removes keys that were suppressed FOR THIS COMBO
    for (const auto &sk : state.suppressedKeys) {
      // ... remove from pendingEvents_ ...
    }
  }
  executeKeyAction(action);
  state.nextKeyIndex = 0;
}
```

**Issue:**
- Only flushes keys that match `suppressedKeys` for that specific combo
- KEY_ENTER doesn't match because:
  - It wasn't in the trigger sequence
  - It wasn't marked as suppressed by that combo
  - It was just queued preventatively

### The Final Emit: Never Reached (Line 441)

**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp#L434-L441)

```cpp
if (ev.type == EV_KEY && (ev.code == KEY_ENTER || ev.code == KEY_KPENTER)) {
  logToFile("Processing ENTER key (code " + std::to_string(ev.code) + 
            ", value " + std::to_string(ev.value) + ") -> uinput",
            LOG_INPUT);
}

std::lock_guard<std::mutex> lock(uinputMutex_);
int rc = libevdev_uinput_write_event(uinputDev_, ev.type, ev.code, ev.value);
```

**Only reached if:**
- Function doesn't return early at line 402 (combo matched)
- Function doesn't return early at line 417/422 (queued)
- Function reaches here naturally

**For KEY_ENTER:** Returns early at line 422 → Never logs, never emits ✗  
**For KEY_KPENTER:** Reaches here → Logs at line 436, emits at line 441 ✓

---

## Where KEY_ENTER is Used

### 1. In Macro Definition (MacroConfig.cpp)

**File:** [daemon/src/MacroConfig.cpp](daemon/src/MacroConfig.cpp#L10-L11)

```cpp
defaultMacros.push_back(
    KeyAction{KeyTrigger{{{BTN_FORWARD, 1, false}}},
              {{KEY_ENTER, 1}, {KEY_ENTER, 0}},  // ← KEY_ENTER used here
              "Triggering mouse forward button (press) → Enter",
              nullptr});
```

**Impact:**
- KEY_ENTER is an OUTPUT of the macro (not a TRIGGER)
- When BTN_FORWARD is pressed, the macro OUTPUTS KEY_ENTER
- Physical KEY_ENTER presses are unrelated to this macro

### 2. Physical KEY_ENTER Presses

When user presses main keyboard Enter key:
- Signal arrives at grabbed keyboard device (line 87)
- Goes through `processEvent()`
- Doesn't match the macro trigger (which is BTN_FORWARD, not KEY_ENTER)
- Gets caught in queue logic
- Stays queued, never emitted

---

## The Keyboard Grab Enforcement

**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp#L87)

```cpp
if (libevdev_grab(keyboardDev_, LIBEVDEV_GRAB) < 0) {
  std::cerr << "CRITICAL: Failed to grab keyboard: " << strerror(errno)
            << std::endl;
  logToFile("Failed to grab keyboard", LOG_CORE);
  return false;
}
```

**Critical Impact:**
- When keyboard is grabbed, ALL physical input goes to daemon only
- System doesn't receive any keyboard events
- Daemon MUST re-emit them via uinput or they're lost
- If KEY_ENTER gets stuck in queue, it's **completely lost to the system**

---

## Why This Specific Problem

### KEY_ENTER (code 28) factors:

1. ✓ Used in macro (as OUTPUT) → Part of macro config
2. ✗ Not used as macro TRIGGER → Never matches eventConsumed path
3. ✓ Keyboard is grabbed → Gets intercepted
4. ✗ Gets queued when combo in progress → Stays in queue
5. ✗ Not in suppressedKeys → Not flushed on combo completion
6. ✗ Never reaches final emit → Lost forever

### KEY_KPENTER (code 96) factors:

1. ✗ Not used in macro at all → Not in config
2. ✗ Not used as macro TRIGGER → Never matches eventConsumed path
3. ✓ Keyboard is grabbed → Gets intercepted
4. ✗ Doesn't get queued → Not part of any combo state
5. N/A (never queued) → Flush logic doesn't apply
6. ✓ Reaches final emit → Works ✓

**The Difference:** KEY_ENTER appears in macro config, making it relevant to combo state tracking. When combos are in progress, KEY_ENTER gets queued as a preventative measure but then orphaned.

---

## Evidence from Code Structure

1. **Combo matching logic** (lines 285-405):
   - Tests ALL macros (which include KEY_ENTER action)
   - Tracks combo progress
   - Tracks which keys are suppressed

2. **Queue logic** (lines 408-428):
   - Assumes all pending events belong to in-progress combos
   - Queues anything that arrives during combo matching
   - **Doesn't distinguish between:**
     - Keys that are part of the combo
     - Keys that are just in the queue for ordering
     - Keys that should be passed through

3. **Flush logic** (lines 336-357, 391-404, 424-427):
   - Tries to flush suppressed keys
   - But uses weak matching (only finds exact suppressedKeys)
   - **Problem:** KEY_ENTER doesn't match suppressedKeys because:
     - It wasn't in the trigger sequence
     - It was added to queue by line 420 (preventative)
     - Not explicitly marked as suppressed

---

## Proof of Concept Scenario

```
Time    Event                   pendingEvents_      anyComboInProgress
─────────────────────────────────────────────────────────────────────
T0      User presses Ctrl       []                  false

T1      Ctrl matches combo      [Ctrl]              true
        start (combo step 1)    (suppressed)

T2      User presses Enter      [Ctrl, Enter]       true
        (main keyboard)         ← QUEUED! Line 420

T3      Combo continues...      [Ctrl, Enter]       true

T4      Combo completes         [Enter] ← Ctrl      false
                                removed, but not
                                Enter! (not in
                                suppressedKeys)

T5      No more combos          [Enter] ← ORPHANED  false
        in progress             never flushed

Result: Enter key lost forever in queue
```

---

## Summary Table: The Complete Picture

| Component | Location | KEY_ENTER Behavior | KEY_KPENTER Behavior |
|-----------|----------|---|---|
| **In macro definition** | MacroConfig.cpp:10-11 | ✓ Used as OUTPUT | ✗ Not used |
| **As macro trigger** | All macros | ✗ Not a trigger | ✗ Not a trigger |
| **Combo matching** | InputMapper.cpp:285 | Matches macro context | No combo context |
| **eventConsumed** | InputMapper.cpp:301 | false (not trigger) | false (not trigger) |
| **Queue logic trigger** | InputMapper.cpp:415 | Can be TRUE | Usually FALSE |
| **Gets queued** | InputMapper.cpp:420 | ✓ YES (if combo in progress) | ✗ NO |
| **Early return** | InputMapper.cpp:422 | ✓ Returns early | ✗ Continues |
| **Final emit reached** | InputMapper.cpp:441 | ✗ NO | ✓ YES |
| **Flush logic** | InputMapper.cpp:356 | ✗ Not flushed (not suppressed) | N/A |
| **Works** | Functional test | ✗ NO | ✓ YES |

---

## Recommendation

This is a **design flaw** in the queue management system. The fix should:

1. **Better distinguish between queue purposes:**
   - Keys suppressed by the combo (flush with combo)
   - Keys queued preventatively (flush when combo breaks or timeout)

2. **Or:** Exclude KEY_ENTER from the queue system entirely (it's critical input)

3. **Or:** Implement a proper flush mechanism that doesn't rely on suppressedKeys matching

The root issue is that the queue logic and flush logic have different assumptions about what should be queued and what should be flushed.

---

## All Referenced Files

- [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp) - Main event processing
  - Line 87: Keyboard grab
  - Line 285-405: Combo matching (eventConsumed logic)
  - Line 408-428: Queue management (KEY_ENTER gets stuck here)
  - Line 336-357: Flush logic (incomplete for KEY_ENTER)
  - Line 434-441: Final emit (never reached for KEY_ENTER)

- [daemon/src/MacroConfig.cpp](daemon/src/MacroConfig.cpp)
  - Line 10-11: KEY_ENTER used in default macro

- [daemon/include/InputMapper.h](daemon/include/InputMapper.h)
  - Line 35: suppressedKeys member
  - Line 47-56: PendingEvent structure

---

## Verification Steps for User

To confirm this hypothesis:

1. Add logging at line 420 showing KEY_ENTER being queued
2. Add logging at line 356 showing what keys are removed from queue
3. Check logs when pressing KEY_ENTER - see if it appears in queue operations
4. Compare with KEY_KPENTER - should not appear in queue operations
5. Check if "Processing ENTER key" log appears for physical KEY_ENTER presses
   - If missing: KEY_ENTER never reaches line 434 (stuck in queue)
   - If present: Something else is blocking it

Expected log pattern for broken KEY_ENTER:
```
(no "Processing ENTER key (code 28...)" message)
(but might see in "Flushing pending events" operations)
```

Expected log pattern for working KEY_KPENTER:
```
Processing ENTER key (code 96, value 1) -> uinput
Processing ENTER key (code 96, value 0) -> uinput
```

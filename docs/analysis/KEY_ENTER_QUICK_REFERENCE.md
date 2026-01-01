# Quick Reference: All Code Locations for KEY_ENTER vs KEY_KPENTER Issue

## File: daemon/src/InputMapper.cpp

| Line(s) | What Happens | Impact on KEY_ENTER | Impact on KEY_KPENTER |
|---------|-------------|---|---|
| 87 | Keyboard grabbed with `libevdev_grab()` | ✓ Intercepted | ✓ Intercepted |
| 214-220 | `emit()` function - writes single key to uinput | Used in emit path | Used in emit path |
| 223-231 | `emitSequence()` function - writes key sequence | Used for macro injection | Not used |
| 243 | `processEvent()` function entry | ✓ Called | ✓ Called |
| 247-249 | Update ctrlDown state | Tracked | Tracked |
| 251-253 | Filter out MSC events | Doesn't filter KEY_ENTER | Doesn't filter KEY_KPENTER |
| 255-267 | Debug Ctrl+V logging | Unrelated | Unrelated |
| 269-279 | G-Key sequence detection | KEY_ENTER not a G-key | KEY_KPENTER not a G-key |
| **285-405** | **COMBO MATCHING LOGIC** | **✗ ISSUE STARTS HERE** | ✓ Bypassed |
| 285-294 | Entry to combo matching (if skipMacros false and EV_KEY) | ✓ Enters | ✓ Enters |
| 303-367 | Loop through all combos for current app | Tests all (including KEY_ENTER macro) | Tests all (but KEY_KPENTER not in any) |
| 313-326 | Check if key matches combo step | No match (KEY_ENTER not a trigger) | No match |
| 301 | `eventConsumed = false` when no match | ✓ Set to false | ✓ Set to false |
| 336-357 | Check if combo complete, flush suppressed | Only flushes suppressed keys | N/A |
| 381-405 | If eventConsumed=true, handle and return | Returns if matched, else continue | Continues (not matched) |
| **402** | **EARLY RETURN if eventConsumed** | N/A (false) | N/A (false) |
| **408-428** | **QUEUE MANAGEMENT LOGIC** | **✓ GETS QUEUED HERE** | ✗ Usually bypassed |
| 410-412 | Check if currentlySuppressing (pendingEvents_ non-empty) | ✓ True if combo has suppressed keys | ✗ False (nothing suppressed) |
| 413 | Check if anyComboInProgress | ✓ True if combo still advancing | ✗ Usually false |
| **415** | **If currentlySuppressing && anyComboInProgress** | **✓ TRUE → Gets queued** | **✗ FALSE → Continues** |
| **417-420** | **Queue the event** | **✓ KEY_ENTER added to pendingEvents_** | ✗ Not added |
| **422** | **EARLY RETURN if queued** | **✓ RETURNS - NEVER REACHES FINAL EMIT** | ✗ Continues |
| 424-427 | Else: Flush pending events | Not applicable (returned early) | Might execute but pendingEvents_ empty |
| **434-441** | **FINAL EMIT TO UINPUT** | **✗ NEVER REACHED (returned at 422)** | **✓ REACHED AND WORKS** |
| 434-436 | Log KEY_ENTER or KEY_KPENTER if it's one of these | ✗ Never logged (never reached) | ✓ Logged |
| 441 | `libevdev_uinput_write_event(uinputDev_, ev.type, ev.code, ev.value)` | ✗ Not executed (returned early) | ✓ Executed |
| 512 | `executeKeyAction()` called when combo completes | Used for BTN_FORWARD → KEY_ENTER injection | Not used |

---

## File: daemon/src/MacroConfig.cpp

| Line(s) | What Happens | Relevance |
|---------|-------------|-----------|
| 10-13 | Default macro: BTN_FORWARD (press) → KEY_ENTER | **KEY_ENTER defined as macro OUTPUT** |
| 15-19 | Default macro: G5 (press) → Ctrl+V | Uses emitSequence |
| 21-24 | Default macro: G6 (press) → Ctrl+C | Uses emitSequence |
| 28-38 | Terminal app macros | No KEY_ENTER |
| 40-47 | VS Code app macros | No KEY_ENTER |
| 49-57 | Chrome app macros (Ctrl+V with callback) | Ctrl+V has special handler |
| 59 | Other (default) app macros | Gets default macros (including KEY_ENTER one) |

**Impact:**
- KEY_ENTER appears only as a MACRO OUTPUT (action)
- Never as a MACRO TRIGGER (condition)
- This makes it invisible to normal combo matching
- But it affects how combo state is tracked globally

---

## File: daemon/include/InputMapper.h

| Line(s) | Structure/Member | Purpose | Impact |
|---------|------------------|---------|--------|
| 31-36 | `struct ComboState` | Tracks progress of one combo | Stores which keys in sequence matched |
| 35 | `std::vector<std::pair<uint16_t, uint8_t>> suppressedKeys` | Tracks keys actually suppressed by THIS combo | KEY_ENTER NOT in this list (doesn't match trigger) |
| 39-46 | `struct KeyTrigger` | Defines combo sequence and which keys to suppress | KEY_ENTER not here (only in actions) |
| 47-56 | `std::vector<PendingEvent> pendingEvents_` | Queue of delayed key events | **KEY_ENTER ends up here** |
| 47 | `struct PendingEvent` (implicit) | Event queued for later | Where KEY_ENTER is stuck |
| 118 | `std::map<AppType, std::map<size_t, ComboState>> comboProgress_` | Tracks progress of all combos per app | Used to determine anyComboInProgress |

---

## Critical Path for Physical KEY_ENTER Press

```
Location: InputMapper.cpp
Function: processEvent(input_event ev, ...)

Entry at 243:
  processEvent(KEY_ENTER event, ...)

Line 285:
  if (!skipMacros && ev.type == EV_KEY) {  // ✓ TRUE - KEY_ENTER is EV_KEY

Line 288-294:
  Get currentApp
  Find appMacros_[currentApp]

Line 303:
  for each combo in appMacros_[currentApp] {  // Tests BTN_FORWARD→KEY_ENTER macro
    
    Line 313-326:
      expectedCode = KEY_ENTER (from action, not trigger!)
      ev.code = KEY_ENTER
      if (expectedCode == ev.code && expectedState == ev.value) {
        ✗ FALSE - Because KEY_ENTER never matches the TRIGGER condition
        // The trigger is BTN_FORWARD, not KEY_ENTER!
      }
  }

Line 369-371:
  anyComboInProgress = check if any combo.nextKeyIndex > 0
  // Could be TRUE if some other key started a combo

Line 381:
  if (eventConsumed) {  // ✗ FALSE - no combo matched
    // Skip this block
  }

Line 408:
  {
    std::lock_guard<std::mutex> lock(pendingEventsMutex_);
    currentlySuppressing = !pendingEvents_.empty();  // ✓ TRUE if any keys queued
  }

Line 413-415:
  if (currentlySuppressing && anyComboInProgress) {  // ✓ Both true?
    
    Line 419-420:
      pendingEvents_.push_back(KEY_ENTER event);  // ✓ ADD TO QUEUE
      return;  // ✓ EARLY EXIT - DON'T CONTINUE!
  }

[Function returns, never reaches line 434-441]
[KEY_ENTER stuck in queue]
```

---

## Critical Path for Physical KEY_KPENTER Press

```
Location: InputMapper.cpp
Function: processEvent(input_event ev, ...)

Entry at 243:
  processEvent(KEY_KPENTER event, ...)

Line 285:
  if (!skipMacros && ev.type == EV_KEY) {  // ✓ TRUE - KEY_KPENTER is EV_KEY

Line 288-294:
  Get currentApp
  Find appMacros_[currentApp]

Line 303:
  for each combo in appMacros_[currentApp] {  // Tests all macros
    
    Line 313-326:
      Check each combo step
      ✗ KEY_KPENTER not in any trigger
      ✗ KEY_KPENTER not in any action (only KEY_ENTER is)
      // No match
  }

Line 369-371:
  anyComboInProgress = check if any combo.nextKeyIndex > 0
  // Depends on other keys pressed

Line 381:
  if (eventConsumed) {  // ✗ FALSE - no combo matched
    // Skip this block
  }

Line 408:
  {
    std::lock_guard<std::mutex> lock(pendingEventsMutex_);
    currentlySuppressing = !pendingEvents_.empty();
  }

Line 413-415:
  if (currentlySuppressing && anyComboInProgress) {
    // Might be FALSE because:
    // - KEY_KPENTER not part of any macro
    // - pendingEvents_ might be empty
    // - Or combo might have completed
    
    ✗ FALSE - Skip queue logic
  }

Line 424-427:
  else {
    // Flush pending events if needed
    // Usually nothing to flush for KEY_KPENTER
  }

[Continue to line 434]

Line 434-436:
  if (ev.type == EV_KEY && (ev.code == KEY_ENTER || ev.code == KEY_KPENTER)) {
    logToFile("Processing ENTER key (code 96, value 1)...");  // ✓ LOGGED
  }

Line 441:
  libevdev_uinput_write_event(uinputDev_, KEY_KPENTER, ...);  // ✓ EMITTED

[KEY_KPENTER successfully sent to uinput]
[Application receives the key]
```

---

## Key Code Values

```
#define KEY_ENTER   28   // Main keyboard Enter
#define KEY_KPENTER 96   // Numeric keypad Enter
```

---

## The Queue Problem in Sequence

```
1. Some trigger key is pressed → Added to pendingEvents_ (suppressed)
2. anyComboInProgress = true (combo still matching)
3. Physical KEY_ENTER arrives
4. Line 410-412: currentlySuppressing = true (pendingEvents_ not empty)
5. Line 413: anyComboInProgress = true (still matching combo)
6. Line 415: if (true && true) → TRUE
7. Line 420: KEY_ENTER pushed to pendingEvents_
8. Line 422: return (EARLY EXIT!)
9. Never reaches line 434-441 (final emit)
10. Combo eventually completes
11. Line 356: Only removes keys in suppressedKeys from queue
12. KEY_ENTER not in suppressedKeys (wasn't trigger key)
13. KEY_ENTER remains in queue forever ✗
```

---

## All Suppressed Keys Tracking

The `suppressedKeys` list only contains keys that:
1. Were in the combo trigger sequence
2. Had suppress=true flag
3. Actually matched the combo step

KEY_ENTER doesn't match because:
- It's not in any combo trigger
- It's only in combo actions
- It was added to queue by line 420 (preventative), not because it matched

This mismatch is the bug.

---

## Summary

**The smoking gun:** Lines 410-422 in InputMapper.cpp

The queue logic assumes any event arriving while `currentlySuppressing && anyComboInProgress` needs to be queued. But the flush logic at line 356 only removes events that were explicitly suppressed by the matching combo.

KEY_ENTER:
- Gets queued at line 420 (preventative)
- Never removed from queue (not in suppressedKeys)
- Never reaches final emit at line 441

KEY_KPENTER:
- Usually doesn't enter queue logic
- Falls through to final emit at line 441
- Works fine

**Fix needed:** Better tracking of why events were queued, or special handling for KEY_ENTER to bypass the queue.

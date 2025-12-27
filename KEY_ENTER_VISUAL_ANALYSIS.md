# Visual Architecture: Why KEY_ENTER Fails vs KEY_KPENTER Works

## The Two Input Paths in daemon

```
PHYSICAL KEYBOARD INPUT
        ↓
    ┌───────────────────────────────────────────┐
    │  libevdev grab keyboard device            │ ← Line 87
    │  (InputMapper.cpp:87)                     │
    └─────────────────┬─────────────────────────┘
                      ↓
    ┌───────────────────────────────────────────┐
    │  processEvent(input_event)                │ ← Line 243
    │  (InputMapper.cpp:243)                    │
    └─────────────────┬─────────────────────────┘
                      ↓
        ┌─────────────────────────┐
        │ Combo Matching          │ ← Lines 285-405
        │ (Check all macros)      │
        └─────────────┬───────────┘
                      ↓
        ┌─────────────────────────────────────────────┐
        │  Did event match ANY combo step?            │ ← Line 303-367
        └──────────┬──────────────────────────────────┘
                   ↓
       ┌─────────────────────────────────┐
       │ YES - eventConsumed = true       │  NO - eventConsumed = false
       │                                 │
       └────┬────────────────────────────┴────────┬──────────────┐
            ↓                                     ↓              ↓
      ┌──────────────┐               ┌──────────────────────────────────┐
      │ RETURN       │               │ Check if queuing needed          │
      │ Early exit   │               │ (Lines 408-428)                  │
      │ (Line 402)   │               └──────┬─────────────────────────┘
      └──────────────┘                      ↓
         ↓ (skips)                    ┌─────────────┐
         ║                            │ Queue the   │  ← Gets queued if:
         ║                            │ event?      │  - pendingEvents_ non-empty
         ║                            │ (Line 420)  │  - anyComboInProgress=true
         ║                            │             │  → Returns early! (Line 422)
         ║                            └─────────────┘
         ║                                  ↓
         ║                        ┌─────────────────────┐
         ║                        │ RETURN early        │
         ║                        │ (Line 422)          │
         ║                        │ Event remains in    │
         ║                        │ pendingEvents_      │
         ║                        └─────────────────────┘
         ║                                  ↓
         ║                        ┌─────────────────────────────┐
         ║                        │ CRITICAL ISSUE for KEY_ENTER│
         ║                        │ Never reaches final emit!    │
         ║                        └─────────────────────────────┘
         ║
         ║ CONTINUE (not queued)
         ║      ↓
         └─────→ ┌─────────────────────────────────┐
                 │ Line 434-436: Log ENTER key     │
                 │ if (code == KEY_ENTER ||        │
                 │     code == KEY_KPENTER)        │
                 │ logToFile("Processing...")      │
                 └────────────┬────────────────────┘
                              ↓
                 ┌────────────────────────────────┐
                 │ Line 441: Final emit            │
                 │ libevdev_uinput_write_event()   │
                 │ KEY_KPENTER WORKS HERE ✓        │
                 │ KEY_ENTER NEVER REACHES HERE ✗  │
                 └────────────────────────────────┘
```

---

## Macro Injection Path (When Macro Executes)

```
MACRO TRIGGER (e.g., BTN_FORWARD press)
        ↓
    ┌──────────────────────────────┐
    │ Combo matched!               │
    │ (MacroConfig.cpp:10-11)      │
    │ Triggers: KEY_ENTER inject   │
    └─────────┬────────────────────┘
              ↓
    ┌──────────────────────────────┐
    │ executeKeyAction()            │ ← Line 512
    │ (InputMapper.cpp:512)        │
    └─────────┬────────────────────┘
              ↓
    ┌──────────────────────────────┐
    │ emitSequence()               │ ← Line 223
    │ (InputMapper.cpp:223)        │
    │ {{KEY_ENTER, 1},             │
    │  {KEY_ENTER, 0}}             │
    └─────────┬────────────────────┘
              ↓
    ┌────────────────────────────────────────┐
    │ libevdev_uinput_write_event()          │ ✓ WORKS
    │ for each key in sequence               │
    │ (via uinputMutex_)                     │
    └──────────────────────────────────────┘
```

---

## The Core Problem Visualized

```
SCENARIO 1: KEY_KPENTER (Numeric Keypad Enter)
═══════════════════════════════════════════════

Physical KEY_KPENTER press arrives
              ↓
    No combo matches (never in macro triggers)
              ↓
    eventConsumed = false
              ↓
    pendingEvents_ is empty (nothing was suppressed)
              ↓
    Continue past queuing logic
              ↓
    Reaches line 434-441 logging & emit
              ↓
    ✓ WRITTEN TO UINPUT SUCCESSFULLY ✓
              ↓
    Application receives KEY_KPENTER
              ↓
    WORKS! ✓


SCENARIO 2: KEY_ENTER (Main Keyboard Enter)
═════════════════════════════════════════════

Physical KEY_ENTER press arrives
              ↓
    No combo triggers KEY_ENTER (it's an OUTPUT)
              ↓
    eventConsumed = false
              ↓
    ▼▼▼ PROBLEM: Enters queue logic ▼▼▼
    
    Check if currentlySuppressing:
    - Are pendingEvents_ non-empty?
    - Is anyComboInProgress still true?
              ↓
    IF BOTH YES:
      Queue KEY_ENTER at line 420
      Return at line 422
      ▼▼▼ NEVER REACHES line 441 ▼▼▼
              ↓
    KEY_ENTER STUCK IN QUEUE
    Never flushed (until combo breaks/completes)
              ↓
    ✗ NEVER WRITTEN TO UINPUT ✗
              ↓
    Application never receives KEY_ENTER
              ↓
    FAILS! ✗


SCENARIO 3: Why No Queue Flush?
════════════════════════════════

When does queue get flushed?
- Line 398: After combo completes
- Line 426: When exiting suppression state

But KEY_ENTER flush requires:
- Line 398: ✗ NOT matched by combo (no completion)
- Line 426: ✗ Only flushed if pendingEvents_ was
              from SUPPRESSED keys (line 356)

KEY_ENTER was added to queue at line 420 BECAUSE
of pending state, not because it matched a combo.

Result: Orphaned in queue with no flush mechanism!
```

---

## The Queue State Machine Problem

```
Time → 
═══════════════════════════════════════════════════════════════════

State 1: Initial
  - pendingEvents_ = []
  - anyComboInProgress = false

State 2: User presses some trigger key for a combo
  - Combo matching starts
  - anyComboInProgress = true
  - Some key gets marked shouldSuppress = true
  - Key is added to queue → pendingEvents_ = [triggerKey]
  
State 3: Physical KEY_ENTER arrives
  - eventConsumed = false (doesn't match ANY combo)
  - currentlySuppressing = true (pendingEvents_ non-empty!)
  - anyComboInProgress = true (combo still in progress)
  - Condition at line 415: if (currentlySuppressing && anyComboInProgress)
    ↓ TRUE!
  - KEY_ENTER added to queue → pendingEvents_ = [triggerKey, KEY_ENTER]
  - Return at line 422 (EARLY EXIT!)
  
State 4: Original trigger combo completes
  - Combo matched fully
  - executeKeyAction() called
  - pendingEvents_ flushed... but ONLY for suppressed keys!
  - Example: Only flushes triggerKey that was suppressed
  - KEY_ENTER was queued but NOT in suppressedKeys list
    → REMAINS IN QUEUE!
    
State 5: Deadlock
  - pendingEvents_ = [KEY_ENTER]
  - But no condition to flush it anymore
  - anyComboInProgress = false (original combo done)
  - KEY_ENTER stays queued forever (until next event arrives)
```

---

## Code Logic Flaw

**File:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp)  
**Lines:** 410-428

The problem is in this section:

```cpp
if (currentlySuppressing && anyComboInProgress) {
  // ▼▼▼ ISSUE: Queues KEY_ENTER even though it's NOT
  // suppressed by the in-progress combo! ▼▼▼
  std::lock_guard<std::mutex> lock(pendingEventsMutex_);
  pendingEvents_.push_back(
      {(uint16_t)ev.type, (uint16_t)ev.code, (int32_t)ev.value});
  return;  // ← EARLY EXIT - never reaches final emit!
}
```

**The Logic Assumption:**
"If we're already suppressing something, and a combo is in progress,
 queue this new event to preserve order"

**The Wrong Outcome:**
- It queues ANY event, even unrelated ones like KEY_ENTER
- It doesn't track WHY the event was queued
- When combo completes, only explicitly-suppressed keys are flushed
- Unrelated queued events (like KEY_ENTER) remain orphaned

---

## Why This Doesn't Affect KEY_KPENTER

**KEY_KPENTER** never hits the queue because:

```
User timeline:
1. User presses KEY_KPENTER
2. Macro trigger (e.g., BTN_FORWARD) usually happens separately
3. If KEY_KPENTER arrives and no combo is in progress:
   - eventConsumed = false
   - currentlySuppressing = false (pendingEvents_ empty)
   - Line 415 condition: if (false && anyComboInProgress) → FALSE
   - Skips queue logic!
   - Falls through to line 434-441
   - Emit works ✓
```

But for KEY_ENTER:

```
User timeline:
1. User presses some key that starts a combo (trigger key)
2. That key gets suppressed, added to queue
3. User presses KEY_ENTER (intended for application)
4. pendingEvents_ still contains the trigger key
5. Some combo still in progress
6. Line 415 condition: if (true && true) → TRUE
7. KEY_ENTER gets queued!
8. Returns early - never emitted
9. Combo eventually completes, flushes only trigger keys
10. KEY_ENTER remains in queue forever
```

---

## The Fix Would Require

Option 1: **Special case KEY_ENTER**
```cpp
// Don't queue KEY_ENTER, let it pass through
if (ev.code == KEY_ENTER) {
  emit(ev.type, ev.code, ev.value);
  return;
}
// Then check queue logic for other keys
if (currentlySuppressing && anyComboInProgress) { ... }
```

Option 2: **Better queue tracking**
```cpp
// Track reason for queueing
struct QueuedEvent {
  input_event ev;
  bool suppressed;  // Was this event suppressed?
  size_t comboIdx;  // Which combo caused suppression?
};

// Then flush based on which combo completes
```

Option 3: **Universal flush**
```cpp
// After any combo state change, flush pending if safe
if (pendingEvents_.non_empty() && no_active_combos) {
  flushPending();
}
```

---

## File Dependencies

```
MacroConfig.cpp (Defines macros with KEY_ENTER)
        ↓
        ↓ Registers in InputMapper
        ↓
InputMapper.cpp (Contains processEvent logic)
    ├── Uses suppressedKeys (InputMapper.h line 35)
    ├── Uses pendingEvents_ (InputMapper.h line 47)
    ├── Uses libevdev_grab (line 87)
    ├── Uses combo matching (lines 285-405)
    ├── Uses queue logic (lines 408-428)
    ├── Calls emit() and emitSequence()
    └── Final emit at line 441
        ↓
        ↓ Both use uinput
        ↓
    uinput device written to
        ↓
    System receives event
```

---

## Detection in Logs

To confirm this hypothesis, look for:

```log
# If KEY_ENTER is being queued:
"Flushing pending events (mismatch/broken combo)"  ← Should show KEY_ENTER
or
"Flushing remaining after complete"                ← Might show KEY_ENTER

# If KEY_ENTER never reaches logging:
NO "Processing ENTER key (code 28, ...)" message   ← KEY_ENTER stuck in queue

# Compare with KEY_KPENTER:
"Processing ENTER key (code 96, ...)"              ← ALWAYS appears
```

The logs are your diagnostic tool. If KEY_ENTER is present in "Flushing pending" 
but not in "Processing ENTER key", it's being queued and never reaching the final emit.

---

## Summary

| Aspect | KEY_ENTER (28) | KEY_KPENTER (96) |
|--------|---|---|
| **Used as macro action?** | ✓ YES (MacroConfig.cpp:10) | ✗ NO |
| **Used as combo trigger?** | ✗ NO (only as output) | ✗ NO |
| **Gets queued?** | ✓ LIKELY (line 420 condition met) | ✗ NO |
| **Flushed properly?** | ✗ NO (not in suppressedKeys) | N/A |
| **Reaches final emit?** | ✗ NO (returns early @ line 422) | ✓ YES |
| **Works?** | ✗ NO | ✓ YES |

**Root Cause:** The queue logic at lines 408-428 treats all in-progress events equally, 
but the flush logic at lines 336-357 only removes explicitly-suppressed keys. 
Physical KEY_ENTER gets queued as a preventative measure but never flushed, 
creating a deadlock.

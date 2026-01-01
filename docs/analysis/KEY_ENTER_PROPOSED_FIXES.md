# Proposed Fixes for KEY_ENTER Queue Deadlock

## Problem Summary

Physical KEY_ENTER (code 28) gets stuck in the `pendingEvents_` queue when a combo is in progress, while KEY_KPENTER (code 96) works fine because it never enters the queue.

**Root cause:** Lines 408-428 in `InputMapper.cpp` - Queue logic doesn't match flush logic.

---

## Solution Options

### Option 1: Exclude KEY_ENTER from Queue (RECOMMENDED - Simplest)

**Principle:** KEY_ENTER is critical input; don't queue it.

**Location:** [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp#L408-L428)

**Current code:**
```cpp
if (currentlySuppressing && anyComboInProgress) {
  // Still matching some other potential combo, keep queueing
  std::lock_guard<std::mutex> lock(pendingEventsMutex_);
  pendingEvents_.push_back(
      {(uint16_t)ev.type, (uint16_t)ev.code, (int32_t)ev.value});
  return;
}
```

**Proposed fix:**
```cpp
if (currentlySuppressing && anyComboInProgress) {
  // Still matching some other potential combo, keep queueing
  // BUT: Don't queue critical keys like KEY_ENTER (code 28)
  if (ev.code == KEY_ENTER) {
    // KEY_ENTER is critical - emit immediately
    emit(ev.type, ev.code, ev.value);
    return;
  }
  
  std::lock_guard<std::mutex> lock(pendingEventsMutex_);
  pendingEvents_.push_back(
      {(uint16_t)ev.type, (uint16_t)ev.code, (int32_t)ev.value});
  return;
}
```

**Pros:**
- Simple, one-line condition
- Doesn't change architecture
- Fixes the specific issue
- Minimal risk

**Cons:**
- Might break order preservation if KEY_ENTER should wait
- Requires identifying which keys are "critical"
- Could emit KEY_ENTER before earlier queued keys

**Risk Level:** LOW - Key ordering less critical for KEY_ENTER specifically

---

### Option 2: Better Queue Tracking (RECOMMENDED - More Robust)

**Principle:** Track why each event was queued; flush correctly.

**Location:** [daemon/include/InputMapper.h](daemon/include/InputMapper.h#L47-L56)

**Current queue structure:**
```cpp
struct PendingEvent {
  uint16_t type;
  uint16_t code;
  int32_t value;
};

std::vector<PendingEvent> pendingEvents_;
```

**Proposed enhanced structure:**
```cpp
struct PendingEvent {
  uint16_t type;
  uint16_t code;
  int32_t value;
  bool suppressedByCombo;  // Was this suppressed by a combo trigger?
  size_t comboIdx;         // Which combo suppressed it (if applicable)?
};

std::vector<PendingEvent> pendingEvents_;
std::map<size_t, std::vector<size_t>> comboToQueuedEvents_;  // Track which queued events belong to which combo
```

**Then in queue logic** [InputMapper.cpp](daemon/src/InputMapper.cpp#L417-L420):

```cpp
std::lock_guard<std::mutex> lock(pendingEventsMutex_);
pendingEvents_.push_back({
    (uint16_t)ev.type, 
    (uint16_t)ev.code, 
    (int32_t)ev.value,
    false,           // Not suppressed by any combo
    SIZE_MAX         // Not associated with any specific combo
});
```

**Then in flush logic** [InputMapper.cpp](daemon/src/InputMapper.cpp#L336-L357):

```cpp
if (state.nextKeyIndex == action.trigger.keyCodes.size()) {
  logToFile("COMBO COMPLETE: " + action.logMessage, LOG_AUTOMATION);

  if (action.trigger.hasSuppressedKeys) {
    completedSuppressing = true;
    std::lock_guard<std::mutex> lock(pendingEventsMutex_);
    
    // Remove only the suppressed keys
    for (const auto &sk : state.suppressedKeys) {
      auto it = std::find_if(
          pendingEvents_.begin(), pendingEvents_.end(),
          [&sk](const PendingEvent &pe) {
            return pe.code == sk.first && pe.value == sk.second && pe.suppressedByCombo;
          });
      if (it != pendingEvents_.end()) {
        pendingEvents_.erase(it);
      }
    }
    
    // Also flush any events that were queued only for ordering during this combo
    // but weren't explicitly suppressed by it
    auto it = pendingEvents_.begin();
    while (it != pendingEvents_.end()) {
      if (!it->suppressedByCombo) {
        // This event was queued preventatively - safe to emit now if combo done
        emit(it->type, it->code, it->value);
        it = pendingEvents_.erase(it);
      } else {
        ++it;
      }
    }
    
    state.suppressedKeys.clear();
  }

  executeKeyAction(action);
  state.nextKeyIndex = 0;
}
```

**Pros:**
- Distinguishes between different reasons for queueing
- Allows intelligent flush decisions
- Future-proof for more complex scenarios
- Keeps key ordering when needed

**Cons:**
- More code changes
- Additional memory per queued event
- More complex logic

**Risk Level:** MEDIUM - Changes core data structure

---

### Option 3: Timeout-Based Flush (Safer but More Complex)

**Principle:** Unblock queue after a timeout if combo never completes.

**Location:** [daemon/include/InputMapper.h](daemon/include/InputMapper.h#L118+)

**Add to private members:**
```cpp
std::map<size_t, std::chrono::steady_clock::time_point> comboStartTimes_;
static constexpr std::chrono::milliseconds COMBO_TIMEOUT{500};  // 500ms timeout
```

**In combo matching logic** [InputMapper.cpp](daemon/src/InputMapper.cpp#L313):
```cpp
if (expectedCode == ev.code && expectedState == ev.value) {
  // Match! Advance state
  state.nextKeyIndex++;
  eventConsumed = true;
  
  // Record when this combo started (if first step)
  if (state.nextKeyIndex == 1) {
    comboStartTimes_[comboIdx] = std::chrono::steady_clock::now();
  }
  // ... rest of matching logic
}
```

**In queue logic** [InputMapper.cpp](daemon/src/InputMapper.cpp#L408-L428):
```cpp
// Check if any combos have timed out
for (auto &pair : comboStartTimes_) {
  size_t idx = pair.first;
  auto startTime = pair.second;
  auto elapsed = std::chrono::steady_clock::now() - startTime;
  
  if (elapsed > COMBO_TIMEOUT) {
    // Combo timeout - flush any queued events
    std::lock_guard<std::mutex> lock(pendingEventsMutex_);
    if (!pendingEvents_.empty()) {
      logToFile("Combo timeout - flushing pending events", LOG_INPUT);
      for (const auto &pe : pendingEvents_) {
        emit(pe.type, pe.code, pe.value);
      }
      pendingEvents_.clear();
    }
    comboProgress_[currentApp][idx].nextKeyIndex = 0;
    comboStartTimes_.erase(idx);
  }
}

if (currentlySuppressing && anyComboInProgress) {
  // Still matching some other potential combo, keep queueing
  std::lock_guard<std::mutex> lock(pendingEventsMutex_);
  pendingEvents_.push_back(
      {(uint16_t)ev.type, (uint16_t)ev.code, (int32_t)ev.value});
  return;
}
```

**Pros:**
- Handles deadlock automatically
- Doesn't change fundamental logic
- Allows combos to work while protecting against orphaned keys

**Cons:**
- Adds timeout latency
- More complex timer management
- Could cause unexpected flushes if tuning is off

**Risk Level:** MEDIUM - Adds time-dependent behavior

---

## Recommended Solution: Option 1 + Option 2

Use **Option 1 as immediate fix** (simple, low risk) and **Option 2 as long-term improvement** (better architecture).

### Immediate Fix (Option 1)

Edit [daemon/src/InputMapper.cpp](daemon/src/InputMapper.cpp#L413-L422):

```cpp
if (currentlySuppressing && anyComboInProgress) {
  // Critical keys like KEY_ENTER should never be queued
  if (ev.code == KEY_ENTER || ev.code == KEY_KPENTER) {
    // These are too important to delay - emit immediately
    emit(ev.type, ev.code, ev.value);
    return;
  }
  
  // Still matching some other potential combo, keep queueing other keys
  std::lock_guard<std::mutex> lock(pendingEventsMutex_);
  pendingEvents_.push_back(
      {(uint16_t)ev.type, (uint16_t)ev.code, (int32_t)ev.value});
  return;
}
```

**Testing:**
1. Press BTN_FORWARD (starts combo)
2. While holding, press main keyboard Enter
3. Should work now (enters emitted immediately)
4. Verify no ordering issues with other keys

---

## Alternative: Why Not Remove Queueing Entirely?

**Question:** Why queue at all if it causes deadlocks?

**Answer:** Order preservation.

When a combo is in progress and suppressing keys:
- User presses suppressed key (e.g., Ctrl)
- User presses normal key (e.g., A)
- Combo might need all keys in exact order
- If Key A is emitted before combo completes, combo might break
- So Key A must be queued to preserve order

However, this causes KEY_ENTER to be orphaned because:
1. Queueing logic assumes "queue everything for safety"
2. Flush logic assumes "only remove suppressed keys"
3. Mismatch → Orphaned keys

**The real fix:** Make these assumptions consistent (Option 2)

---

## Code Review Checklist

After implementing a fix:

- [ ] Physical KEY_ENTER press works
- [ ] Physical KEY_KPENTER still works
- [ ] BTN_FORWARD → KEY_ENTER macro still works
- [ ] Other macros still work correctly
- [ ] No key ordering violations in complex combos
- [ ] No memory leaks from queue
- [ ] Logs show proper key flow
- [ ] Test with rapid key pressing
- [ ] Test with overlapping combos

---

## Testing Script

```bash
#!/bin/bash
# Test KEY_ENTER vs KEY_KPENTER

echo "Starting daemon test..."

# Test 1: Physical KEY_ENTER
echo "Test 1: Press main Enter key"
sleep 2
# [Press main Enter key - should type in terminal]

# Test 2: Physical KEY_KPENTER  
echo "Test 2: Press keypad Enter key"
sleep 2
# [Press keypad Enter key - should type in terminal]

# Test 3: Macro trigger then KEY_ENTER
echo "Test 3: Hold Ctrl, press main Enter"
sleep 2
# [Hold Ctrl (starts combo), then press main Enter - should NOT be stuck]

# Test 4: BTN_FORWARD macro
echo "Test 4: Press mouse forward button"
sleep 2
# [Press mouse forward button - should emit Enter via macro]

echo "All tests complete"
```

---

## Verification via Logs

After fix, logs should show:

```
Test 1: KEY_ENTER physical press
  Processing ENTER key (code 28, value 1) -> uinput
  Processing ENTER key (code 28, value 0) -> uinput
  [Should appear - means it reached final emit]

Test 2: KEY_KPENTER physical press
  Processing ENTER key (code 96, value 1) -> uinput
  Processing ENTER key (code 96, value 0) -> uinput
  [Should always appear]

Test 3: Ctrl + physical KEY_ENTER
  Combo progress: ... (for Ctrl)
  Processing ENTER key (code 28, value 1) -> uinput  [NEW - shows KEY_ENTER not queued]
  Processing ENTER key (code 28, value 0) -> uinput
  [Should show KEY_ENTER being processed, not queued]

Test 4: BTN_FORWARD macro
  Combo COMPLETE: mouse forward button → Enter
  [Processing KEY_ENTER via macro injection]
  [Both keyboard and macro KEY_ENTER should work]
```

---

## Implementation Priority

1. **CRITICAL:** Option 1 (immediate fix for KEY_ENTER)
2. **IMPORTANT:** Add logging to verify queue behavior
3. **MEDIUM:** Option 2 (refactor for robustness)
4. **NICE-TO-HAVE:** Option 3 (timeout safety net)

---

## Related Issues to Watch

1. **Key ordering:** Some combos might require exact key order
   - Fix: Use Option 2 to distinguish suppressed vs preventative queueing

2. **Performance:** More frequent flushing
   - Fix: Use efficient algorithms (already using vector, could use deque)

3. **Timing issues:** If KEY_ENTER emitted too early
   - Fix: Verify with rapid key tests
   - May need slight delays

4. **Mouse button handling:** BTN_FORWARD still needs to work
   - Fix: Shouldn't be affected by KEY_ENTER special case

---

## Summary

**The Problem:** Queue logic and flush logic don't match, orphaning KEY_ENTER.

**Quick Fix:** Don't queue critical keys like KEY_ENTER.

**Long-term Fix:** Track queueing reasons and flush intelligently.

**Implementation:** Start with Option 1, add Option 2 infrastructure.

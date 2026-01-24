# Technical Debt Notes

## ForceForegroundWindow Duplication

**Status**: Accepted (Minor debt)

**Files**:
- `AppDelegate.cpp` (line 20-35)
- `main.cpp` (line 24-39)

**Reason for duplicate**:
- Function is stable, unlikely to change
- Only 2 usages
- Each file remains self-contained
- 15 lines - low overhead

**When to refactor**:
- If more window utility functions are needed
- Then create `WindowUtils.h` or add to `OpenKeyHelper.h`

---

## Global Namespace Pollution

**Status**: Active (Significant debt, workaround in place)

**Date Logged**: 2026-01-13

### The Issue

Legacy header files contain `using namespace std;` at the **global scope**. This pollutes the global namespace and causes name collisions with modern C++ libraries that use identifiers also present in `std::` (e.g., `byte`, `data`, `size`, `move`, etc.).

### Impact

When attempting to integrate modern libraries such as:
- **toml++** (TOML parser)
- **nlohmann::json** (JSON library)
- **Other header-only C++17/20 libraries**

The compiler encounters ambiguous symbol errors because names from `std::` conflict with library-defined types or the Windows SDK (e.g., `std::byte` vs `Windows.h` `byte`).

### Culprit Files

| File | Location |
|------|----------|
| `stdafx.h` | `Sources/OpenKey/win32/OpenKey/OpenKey/` |
| `DataType.h` | `Sources/OpenKey/engine/` |
| `Vietnamese.h` | `Sources/OpenKey/engine/` |
| `Macro.h` | `Sources/OpenKey/engine/` |
| `ConvertTool.h` | `Sources/OpenKey/engine/` |
| `SmartSwitchKey.h` | `Sources/OpenKey/engine/` |
| `ExcludedAppsDialog.h` | `Sources/OpenKey/win32/OpenKey/OpenKey/` |

### Current Workaround

We are using the **PIMPL (Pointer to Implementation) / Opaque Pointer Idiom** to isolate new modules from the legacy headers.

**Example**: `ConfigManager` uses PIMPL to encapsulate `toml++` usage:

```cpp
// ConfigManager.h - Clean public interface, no toml++ includes
class ConfigManager {
public:
    static ConfigManager& instance();
    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue);
    // ...
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// ConfigManager.cpp - Implementation includes toml++ in isolation
#include "ConfigManager.h"
#include <toml++/toml.hpp>  // Safe: not exposed to legacy headers

class ConfigManager::Impl {
    toml::table config;  // toml++ types hidden from public API
    // ...
};
```

This approach works but adds complexity and maintenance overhead.

### Refactoring Strategy (Future)

**Goal**: Remove `using namespace std;` from **all header files**.

**Method**: Apply the **"Boy Scout Rule"**
> *"Leave the code cleaner than you found it."*

Instead of a risky "Big Bang" refactor, incrementally clean up files:

1. **When touching a file for another feature**, also fix its namespace usage.
2. **Action per file**:
   - Remove `using namespace std;` from the header.
   - Add explicit `std::` prefix to all standard library types:
     - `string` → `std::string`
     - `vector` → `std::vector`
     - `map` → `std::map`
     - `wstring` → `std::wstring`
     - etc.
   - Update all `.cpp` files that include the modified header if they relied on the implicit `using`.
3. **Test thoroughly** after each file change.

**Priority Order** (based on dependency depth):
1. `stdafx.h` - Precompiled header, included everywhere (HIGH IMPACT)
2. `DataType.h` - Core data structures
3. `Vietnamese.h` - Engine core
4. `Macro.h`, `ConvertTool.h`, `SmartSwitchKey.h` - Engine modules
5. `ExcludedAppsDialog.h` - UI layer

> [!WARNING]
> `stdafx.h` is the precompiled header and is included by almost every `.cpp` file. Refactoring it will require changes across the **entire codebase**. Plan for a dedicated refactoring session with full regression testing.

### Acceptance Criteria for Completion

- [ ] No header file contains `using namespace std;` at global scope
- [ ] All standard library types use explicit `std::` prefix in headers
- [ ] Modern libraries can be included directly without PIMPL wrappers
- [ ] Build succeeds on all platforms (Windows, macOS, Linux)
- [ ] All existing tests pass

---

## Quick Convert UX Gap

**Status**: Planned Enhancement (Reviewed)

**Date Logged**: 2026-01-21

### Current Behavior

The Quick Convert feature (`onQuickConvert()` in `AppDelegate.cpp`) currently:
1. Reads text from clipboard
2. Converts encoding via `convertUtil()`
3. Writes converted text back to clipboard
4. Shows `MessageBox` success notification

**Problem**: User must manually press `Ctrl+V` to paste the converted text.

### Desired Behavior

User selects text → presses hotkey → text is:
1. Converted
2. **Auto-pasted** in place
3. **Re-selected** so user can see the result (if ≤150 chars)

### Technical Approach (Refined)

See implementation plan for detailed design.

**Key decisions**:

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Keystroke API | `SendInput` batch | Atomic, reliable, no event dropping |
| Length unit | UTF-16 `wstring.length()` | Windows VK_LEFT uses UTF-16 code units |
| Delay strategy | Adaptive poll | Different apps/machines have different speeds |
| Cutoff for reselect | **150 chars** | Skip reselect for long text |
| Notification | Toast/balloon | Non-blocking, auto-dismiss |
| Clipboard history | Preserve via `CF_EXCLUDE_CLIPBOARD_HISTORY` | Don't pollute Win+V |

**API separation** (per coding.md principles):
```cpp
struct QuickConvertResult {
    int utf16Length;
    bool success;
};
QuickConvertResult quickConvert();  // Helper: pure conversion
// AppDelegate: handles paste, reselect, toast
```

### Files to Modify

| File | Change |
|------|--------|
| `OpenKeyHelper.h` | Add `QuickConvertResult` struct, new function signatures |
| `OpenKeyHelper.cpp` | Refactor `quickConvert()`, add `simulatePaste()`, `reselectText()` |
| `AppDelegate.cpp` | Rewrite `onQuickConvert()`, add `showToast()` |

### Acceptance Criteria

- [ ] Converted text is auto-pasted without user intervention
- [ ] Pasted text is re-selected (highlighted) if ≤150 chars
- [ ] Long text (>150 chars): paste only, show toast with explanation
- [ ] Works in Notepad, VS Code, Word, and browser inputs
- [ ] No blocking dialogs that steal focus
- [ ] Clipboard history (Win+V) is NOT polluted

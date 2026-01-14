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

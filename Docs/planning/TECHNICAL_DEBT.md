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

---
trigger: always_on
---

## run cmd in wsl
## Do not build app, i will do it myself
## Sciter Dialog Subprocesses

### ExitProcess vs PostQuitMessage
- **PHẢI** dùng `ExitProcess(0)` cho Sciter dialog subprocesses
- **KHÔNG** dùng `PostQuitMessage(0)` - sẽ gây Sciter reference counting assertion failure (`_ref_cntr == 0` in sciter-om.h)

### forceForegroundWindow Pattern  
- **KHÔNG** refactor `AttachThreadInput` pattern sang RAII
- Code hiện tại đã được test kỹ, giữ nguyên:
```cpp
if (dwCurrentThread != dwForegroundThread) {
    AttachThreadInput(dwCurrentThread, dwForegroundThread, TRUE);
}
// ... SetForegroundWindow calls ...
if (dwCurrentThread != dwForegroundThread) {
    AttachThreadInput(dwCurrentThread, dwForegroundThread, FALSE);
}
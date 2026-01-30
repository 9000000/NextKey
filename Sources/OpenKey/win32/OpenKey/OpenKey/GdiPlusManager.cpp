#include "stdafx.h"
#include "GdiPlusManager.h"
#include <mutex>

namespace GdiPlusManager {
    static ULONG_PTR s_gdiToken = 0;
    static int s_refCount = 0;
    static std::mutex s_mutex;

    void Init() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_refCount == 0) {
            Gdiplus::GdiplusStartupInput gsi;
            Gdiplus::GdiplusStartup(&s_gdiToken, &gsi, NULL);
        }
        s_refCount++;
    }

    void Shutdown() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_refCount > 0) {
            s_refCount--;
            if (s_refCount == 0 && s_gdiToken != 0) {
                Gdiplus::GdiplusShutdown(s_gdiToken);
                s_gdiToken = 0;
            }
        }
    }

    bool IsInitialized() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_gdiToken != 0;
    }
}

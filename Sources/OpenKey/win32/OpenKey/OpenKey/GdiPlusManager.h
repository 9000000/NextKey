#pragma once
#include "stdafx.h"
#include <gdiplus.h>

namespace GdiPlusManager {
    /**
     * @brief Initializes GDI+ if not already initialized.
     * Increments a reference count.
     */
    void Init();

    /**
     * @brief Decrements the reference count and shuts down GDI+ if count reaches zero.
     */
    void Shutdown();

    /**
     * @brief Checks if GDI+ is currently initialized.
     * @returns true if initialized, false otherwise.
     */
    bool IsInitialized();
}

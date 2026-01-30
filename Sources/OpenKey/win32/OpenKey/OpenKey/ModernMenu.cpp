/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#include "stdafx.h"
#include "ModernMenu.h"
#include "GdiPlusManager.h"
#include <windowsx.h>
#include <dwmapi.h>

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")

// using namespace Gdiplus; // CR-001: Removed to prevent namespace pollution

#define CLASS_NAME_MODERN_MENU _T("NextKeyModernMenu")
#define TIMER_HOVER_SUBMENU 1001
#define WM_SHOW_MODERN_MENU (WM_USER + 1005)

// ULONG_PTR ModernMenu::s_gdiToken = 0; // CR-005: Removed
bool ModernMenu::s_classRegistered = false;
bool ModernMenu::s_isShowing = false;

// Acrylic structures
struct ACCENT_POLICY {
    int AccentState;
    int AccentFlags;
    int GradientColor;
    int AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA {
    int Attrib;
    void* pvData;
    int cbData;
};

ModernMenu::ModernMenu(HINSTANCE hInst) 
    : m_hInst(hInst), m_hWnd(NULL), m_hParent(NULL), m_parentMenu(NULL), m_activeSubMenu(NULL),
      m_width(180), m_height(0), m_hoverIndex(-1), m_selectedIndex(0), m_isRunning(false),
      m_itemHeight(32), m_separatorHeight(8), m_hFont(NULL), m_isSubMenu(false),
      m_hoverTimerId(0), m_lastHoveredForSub(-1)
{
    GdiPlusManager::Init();
    RegisterWindowClass();
}

ModernMenu::~ModernMenu() {
    Clear(); 
    if (m_hFont) DeleteObject(m_hFont);
    if (m_hWnd && IsWindow(m_hWnd)) DestroyWindow(m_hWnd);
    GdiPlusManager::Shutdown();
}

void ModernMenu::AddItem(UINT id, const std::wstring& text, bool checked, bool enabled) {
    m_items.emplace_back(id, text, checked, enabled);
}

ModernMenu* ModernMenu::AddSubMenu(const std::wstring& text) {
    ModernMenuItem item(0, text, false, true);
    item.subMenu = std::make_unique<ModernMenu>(m_hInst);
    item.subMenu->m_parentMenu = this;
    ModernMenu* rawPtr = item.subMenu.get();
    m_items.push_back(std::move(item));
    return rawPtr;
}

void ModernMenu::AddSeparator() {
    m_items.push_back(ModernMenuItem::Separator());
}

void ModernMenu::Clear() {
    // unique_ptr auto-cleans submenus when vector is cleared
    m_items.clear();
}

void ModernMenu::RegisterWindowClass() {
    if (s_classRegistered) return;

    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_SAVEBITS;
    wcex.lpfnWndProc = ModernMenu::WndProc;
    wcex.hInstance = m_hInst;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH); // Fix: No background brush to avoid square layer
    wcex.lpszClassName = CLASS_NAME_MODERN_MENU;
    
    if (RegisterClassEx(&wcex)) {
        s_classRegistered = true;
    }
}

void ModernMenu::EnableAcrylic(HWND hwnd) {
    // V4.2: Direction B Polish - Ensure client area is transparent for Backdrop
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    // 1. Dark Mode first
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark)); 
    
    // 2. Corner Preference second
    DWORD corner = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33, &corner, sizeof(corner));

    // 3. System Backdrop last
    DWORD backdrop = 3; // DWMSBT_TRANSIENTWINDOW (Acrylic)
    DwmSetWindowAttribute(hwnd, 38, &backdrop, sizeof(backdrop)); 
}

UINT ModernMenu::Show(HWND hParent, int x, int y, bool isSubMenu) {
    if (!isSubMenu && s_isShowing) return 0;
    if (!isSubMenu) s_isShowing = true;
    
    m_isSubMenu = isSubMenu;
    m_hParent = hParent;
    m_selectedIndex = 0;
    m_hoverIndex = -1;
    
    HDC hDCWin = GetDC(NULL);
    int nHeight = -MulDiv(10, GetDeviceCaps(hDCWin, LOGPIXELSY), 72);
    if (hDCWin) ReleaseDC(NULL, hDCWin);
    
    // Cleanup previous font if Show() is called multiple times (prevents leak)
    if (m_hFont) {
        DeleteObject(m_hFont);
        m_hFont = NULL;
    }
    
    m_hFont = CreateFont(nHeight, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, 
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                         CLEARTYPE_QUALITY, FF_DONTCARE, _T("Segoe UI Variable Text")); // Modern font

    HDC hdc = CreateCompatibleDC(NULL);
    HGDIOBJ hOldFont = m_hFont ? SelectObject(hdc, m_hFont) : NULL;
    CalculateLayout(hdc);
    if (hOldFont) SelectObject(hdc, hOldFont);
    DeleteDC(hdc);
    
    HMONITOR hMonitor = MonitorFromPoint({x, y}, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
    
    // V4.6: Use named constants for screen margins (defined in ModernMenu.h)
    if (y + m_height > mi.rcWork.bottom - SCREEN_MARGIN_Y) y = mi.rcWork.bottom - m_height - SCREEN_MARGIN_Y;
    if (x + m_width > mi.rcWork.right - SCREEN_MARGIN_X) {
        if (isSubMenu && hParent) {
            RECT rcParent;
            GetWindowRect(hParent, &rcParent);
            x = rcParent.left - m_width - SUBMENU_GAP; 
        } else {
            x = mi.rcWork.right - m_width - SCREEN_MARGIN_X;
        }
    }
    if (y < mi.rcWork.top + SCREEN_MARGIN_Y) y = mi.rcWork.top + SCREEN_MARGIN_Y;
    if (x < mi.rcWork.left + SCREEN_MARGIN_X) x = mi.rcWork.left + SCREEN_MARGIN_X;
    m_hWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW, // REMOVED WS_EX_LAYERED
        CLASS_NAME_MODERN_MENU, _T(""), WS_POPUP,
        x, y, m_width, m_height,
        hParent, NULL, m_hInst, this
    );

    if (!m_hWnd) {
        if (!isSubMenu) s_isShowing = false;
        return 0;
    }

    ShowWindow(m_hWnd, SW_SHOW);
    EnableAcrylic(m_hWnd);
    SetForegroundWindow(m_hWnd);
    
    if (!isSubMenu) {
        RunModalLoop();
        if (IsWindow(m_hWnd)) DestroyWindow(m_hWnd);
        m_hWnd = NULL;
        s_isShowing = false;
    }
    
    return m_selectedIndex;
}

void ModernMenu::RunModalLoop() {
    m_isRunning = true;
    MSG msg;
    while (m_isRunning) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                m_isRunning = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            WaitMessage();
        }
    }
    CloseSubMenu();
}

void ModernMenu::CalculateLayout(HDC hdc) {
    m_height = PADDING_Y;
    int maxWidth = 120;
    for (const auto& item : m_items) {
        if (!item.isSeparator) {
            SIZE sz;
            GetTextExtentPoint32(hdc, item.text.c_str(), (int)item.text.length(), &sz);
            int itemW = sz.cx + PADDING_X * 2 + ICON_WIDTH + (item.subMenu ? 20 : 0);
            if (itemW > maxWidth) maxWidth = itemW;
            m_height += m_itemHeight;
        } else {
            m_height += m_separatorHeight;
        }
    }
    m_width = maxWidth + 20;
    m_height += PADDING_Y;
}

void ModernMenu::OnPaint(HDC hdc) {
    RECT rect;
    GetClientRect(m_hWnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;

    HDC memDC = CreateCompatibleDC(hdc);
    
    // Create a 32-bit DIB section to preserve alpha bits for DWM
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    void* pBits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HGDIOBJ hOldBmp = SelectObject(memDC, hBitmap);

    {
        Gdiplus::Graphics g(memDC);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

        // V4.7: Use named color constants for maintainability
        Gdiplus::Color glassTint(
            (MenuColors::GlassTint >> 24) & 0xFF,
            (MenuColors::GlassTint >> 16) & 0xFF,
            (MenuColors::GlassTint >> 8) & 0xFF,
            MenuColors::GlassTint & 0xFF
        );
        g.Clear(glassTint);

        // Add a very subtle inner highlight border for premium feel
        Gdiplus::Color borderColor(
            (MenuColors::BorderHighlight >> 24) & 0xFF,
            (MenuColors::BorderHighlight >> 16) & 0xFF,
            (MenuColors::BorderHighlight >> 8) & 0xFF,
            MenuColors::BorderHighlight & 0xFF
        );
        Gdiplus::Pen borderPen(borderColor, 1.0f);
        g.DrawRectangle(&borderPen, 0, 0, w - 1, h - 1);

        Gdiplus::Font font(memDC, m_hFont);
        Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 235, 235, 235));
        Gdiplus::SolidBrush subBrush(Gdiplus::Color(255, 140, 140, 140));
        
        int currentY = PADDING_Y;
        for (int i = 0; i < (int)m_items.size(); ++i) {
            const auto& item = m_items[i];
            if (item.isSeparator) {
                int sepY = currentY + m_separatorHeight / 2;
                Gdiplus::Pen sepPen(Gdiplus::Color(60, 255, 255, 255), 1.0f);
                g.DrawLine(&sepPen, (Gdiplus::REAL)PADDING_X, (Gdiplus::REAL)sepY, (Gdiplus::REAL)(w - PADDING_X), (Gdiplus::REAL)sepY);
                currentY += m_separatorHeight;
            } else {
                if (i == m_hoverIndex && item.isEnabled) {
                    Gdiplus::RectF hoverRect((Gdiplus::REAL)6, (Gdiplus::REAL)currentY, (Gdiplus::REAL)(w - 12), (Gdiplus::REAL)m_itemHeight);
                    Gdiplus::SolidBrush hoverBrush(Gdiplus::Color(80, 255, 255, 255));
                    
                    Gdiplus::GraphicsPath hoverPath;
                    float hrad = 5.0f;
                    hoverPath.AddArc(hoverRect.X, hoverRect.Y, hrad*2, hrad*2, 180, 90);
                    hoverPath.AddArc(hoverRect.GetRight() - hrad*2, hoverRect.Y, hrad*2, hrad*2, 270, 90);
                    hoverPath.AddArc(hoverRect.GetRight() - hrad*2, hoverRect.GetBottom() - hrad*2, hrad*2, hrad*2, 0, 90);
                    hoverPath.AddArc(hoverRect.X, hoverRect.GetBottom() - hrad*2, hrad*2, hrad*2, 90, 90);
                    hoverPath.CloseFigure();
                    g.FillPath(&hoverBrush, &hoverPath);
                }
                
                if (item.isChecked) {
                    Gdiplus::SolidBrush dotBrush(Gdiplus::Color(255, 0, 156, 255));
                    g.FillEllipse(&dotBrush, (Gdiplus::REAL)(PADDING_X - 10), (Gdiplus::REAL)(currentY + m_itemHeight/2 - 3), 6.0f, 6.0f);
                }
                
                Gdiplus::RectF textRect((Gdiplus::REAL)(PADDING_X + ICON_WIDTH - 12), (Gdiplus::REAL)currentY, (Gdiplus::REAL)(w - PADDING_X - ICON_WIDTH), (Gdiplus::REAL)m_itemHeight);
                Gdiplus::StringFormat format;
                format.SetAlignment(Gdiplus::StringAlignmentNear);
                format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
                g.DrawString(item.text.c_str(), -1, &font, textRect, &format, &textBrush);

                if (item.subMenu) {
                    Gdiplus::PointF pts[3] = {
                        {(Gdiplus::REAL)(w - 18), (Gdiplus::REAL)(currentY + m_itemHeight/2 - 4)},
                        {(Gdiplus::REAL)(w - 18), (Gdiplus::REAL)(currentY + m_itemHeight/2 + 4)},
                        {(Gdiplus::REAL)(w - 15), (Gdiplus::REAL)(currentY + m_itemHeight/2)}
                    };
                    g.FillPolygon(&subBrush, pts, 3);
                }
                currentY += m_itemHeight;
            }
        }
    }

    // Direct copy to screen. Because we used a DIB with alpha=0 and standard 
    // GDI BitBlt, the OS redirection bitmap will correctly interpret 
    // these bits as transparency against the system backdrop.
    BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, hOldBmp);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
}

void ModernMenu::OnMouseMove(int x, int y) {
    // If mouse is outside this window, check if it's in the active submenu or bridge area
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        if (m_activeSubMenu && m_activeSubMenu->m_hWnd) {
            POINT ptScreen = {x, y};
            ClientToScreen(m_hWnd, &ptScreen);
            
            RECT rcChild;
            GetWindowRect(m_activeSubMenu->m_hWnd, &rcChild);
            if (PtInRect(&rcChild, ptScreen)) return; // Keep submenu open

            // Bridge logic: Check if mouse is in the gap between parent and child
            RECT rcParent;
            GetWindowRect(m_hWnd, &rcParent);
            
            // Determine if submenu is on right or left side
            bool isSubmenuOnRight = rcChild.left >= rcParent.right;
            
            if (isSubmenuOnRight) {
                // Submenu on right: bridge is the gap between parent.right and child.left
                if (ptScreen.x >= rcParent.right && ptScreen.x < rcChild.left) {
                    if (ptScreen.y >= rcParent.top && ptScreen.y <= rcParent.bottom) return;
                }
            } else {
                // Submenu on left: bridge is the gap between child.right and parent.left
                if (ptScreen.x <= rcParent.left && ptScreen.x > rcChild.right) {
                    if (ptScreen.y >= rcParent.top && ptScreen.y <= rcParent.bottom) return;
                }
            }
        }
    }

    int oldHover = m_hoverIndex;
    m_hoverIndex = -1;
    
    int currentY = PADDING_Y;
    for (int i = 0; i < (int)m_items.size(); ++i) {
        int h = m_items[i].isSeparator ? m_separatorHeight : m_itemHeight;
        if (y >= currentY && y < currentY + h) {
            if (!m_items[i].isSeparator && m_items[i].isEnabled) m_hoverIndex = i;
            break;
        }
        currentY += h;
    }
    
    if (m_hoverIndex != oldHover) {
        if (m_hoverTimerId) { KillTimer(m_hWnd, TIMER_HOVER_SUBMENU); m_hoverTimerId = 0; }
        
        if (m_hoverIndex != -1 && m_items[m_hoverIndex].subMenu) {
            m_lastHoveredForSub = m_hoverIndex;
            m_hoverTimerId = (int)SetTimer(m_hWnd, TIMER_HOVER_SUBMENU, 300, NULL);
        } else {
            CloseSubMenu();
        }
        InvalidateRect(m_hWnd, NULL, TRUE);
    }
}

void ModernMenu::CloseSubMenu() {
    if (m_activeSubMenu) {
        if (m_activeSubMenu->m_hWnd) ShowWindow(m_activeSubMenu->m_hWnd, SW_HIDE);
        m_activeSubMenu->CloseSubMenu(); // Recursive
        m_activeSubMenu = NULL;
    }
}

void ModernMenu::OpenSubMenu(int index) {
    if (index < 0 || index >= (int)m_items.size() || !m_items[index].subMenu) return;
    
    CloseSubMenu();
    m_activeSubMenu = m_items[index].subMenu.get();
    
    RECT rc;
    GetWindowRect(m_hWnd, &rc);
    
    int currentY = PADDING_Y;
    for (int i = 0; i < index; ++i) currentY += m_items[i].isSeparator ? m_separatorHeight : m_itemHeight;
    
    // Position submenu to the right with a small gap
    // Bridge logic in OnMouseMove handles this gap for both left and right
    m_activeSubMenu->Show(m_hWnd, rc.right + SUBMENU_GAP, rc.top + currentY, true);
}

void ModernMenu::OnClick(int x, int y) {
    if (m_hoverIndex != -1) {
        if (!m_items[m_hoverIndex].subMenu) {
            // It's a command item, set result and close everything
            m_selectedIndex = m_items[m_hoverIndex].id;
            
            ModernMenu* root = this;
            while(root->m_parentMenu) root = root->m_parentMenu;
            root->m_selectedIndex = m_selectedIndex;
            root->m_isRunning = false;
        } else {
            // It's a submenu item. Manual click should probably keep it open or toggle.
            // For context menu feel, we usually do nothing or just ensure it's open.
            OpenSubMenu(m_hoverIndex);
        }
    }
}

LRESULT CALLBACK ModernMenu::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    ModernMenu* pThis = (ModernMenu*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (ModernMenu*)pCreate->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
        return TRUE;
    }
    
    if (pThis) {
        switch (message) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            pThis->OnPaint(hdc);
            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1; // Handled by DWM/Acrylic
        case WM_ACTIVATE: {
            if (LOWORD(wParam) == WA_INACTIVE) {
                 HWND hNext = (HWND)lParam;
                 bool isInternal = false;
                 if (hNext) {
                     TCHAR className[256];
                     GetClassName(hNext, className, 256);
                     if (_tcscmp(className, CLASS_NAME_MODERN_MENU) == 0) isInternal = true;
                 }
                 
                 // If focus is moving to something NOT part of our menu system, close ALL
                 if (!isInternal) {
                     ModernMenu* root = pThis;
                     while (root->m_parentMenu) root = root->m_parentMenu;
                     root->m_isRunning = false; 
                 }
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            pThis->OnMouseMove(x, y);
            return 0;
        }
        case WM_LBUTTONUP: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            pThis->OnClick(x, y);
            return 0;
        }
        case WM_TIMER:
            if (wParam == TIMER_HOVER_SUBMENU) {
                KillTimer(hWnd, TIMER_HOVER_SUBMENU);
                pThis->m_hoverTimerId = 0;
                pThis->OpenSubMenu(pThis->m_lastHoveredForSub);
            }
            return 0;
        case WM_SETCURSOR:
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return TRUE;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) pThis->m_isRunning = false;
            return 0;
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

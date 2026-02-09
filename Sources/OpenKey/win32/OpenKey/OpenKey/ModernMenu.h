#pragma once

#include "stdafx.h"
#include <vector>
#include <string>
#include <memory>
#include <gdiplus.h>

struct ModernMenuItem {
    UINT id;
    std::wstring text;
    bool isSeparator;
    bool isChecked;
    bool isEnabled;
    std::unique_ptr<class ModernMenu> subMenu; // Smart pointer to child menu
    
    ModernMenuItem(UINT _id, const std::wstring& _text, bool _checked = false, bool _enabled = true)
        : id(_id), text(_text), isSeparator(false), isChecked(_checked), isEnabled(_enabled), subMenu(nullptr) {}
    
    // Move-only semantics for unique_ptr member
    ModernMenuItem(ModernMenuItem&&) = default;
    ModernMenuItem& operator=(ModernMenuItem&&) = default;
        
    static ModernMenuItem Separator() {
        ModernMenuItem item(0, L"");
        item.isSeparator = true;
        return item;
    }
};

// Layout constants
constexpr int CORNER_RADIUS = 8;
constexpr int PADDING_X = 16;
constexpr int PADDING_Y = 8;
constexpr int ICON_WIDTH = 24;

// Screen margin constants (TranslucentTB-like floating look)
constexpr int SCREEN_MARGIN_X = 50;  // Distance from left/right edges
constexpr int SCREEN_MARGIN_Y = 4;   // Distance from top/bottom edges
constexpr int SUBMENU_GAP = 4;       // Gap between parent and child menus

// Color constants (ARGB) - Dual theme support
namespace MenuColors {
    // Dark Theme (Windows dark mode)
    namespace Dark {
        constexpr DWORD GlassTint = 0xAA1E1E1E;      // Semi-transparent dark background
        constexpr DWORD BorderHighlight = 0x28FFFFFF; // Subtle white border
        constexpr DWORD SeparatorLine = 0x3CFFFFFF;   // Separator line
        constexpr DWORD HoverFill = 0x50FFFFFF;       // Hover highlight
        constexpr DWORD TextPrimary = 0xFFEBEBEB;     // Main text color
        constexpr DWORD TextSecondary = 0xFF8C8C8C;   // Arrow/secondary text
        constexpr DWORD AccentDot = 0xFF009CFF;       // Checked item dot
    }
    
    // Light Theme (Windows light mode)
    namespace Light {
        constexpr DWORD GlassTint = 0xFFF5F5F5;      // Opaque light background (for ClearType compatibility)
        constexpr DWORD BorderHighlight = 0x20000000; // Subtle dark border
        constexpr DWORD SeparatorLine = 0x30000000;   // Separator line
        constexpr DWORD HoverFill = 0x40000000;       // Hover highlight (dark overlay)
        constexpr DWORD TextPrimary = 0xFF1E1E1E;     // Dark text
        constexpr DWORD TextSecondary = 0xFF666666;   // Arrow/secondary text
        constexpr DWORD AccentDot = 0xFF0078D4;       // Windows 11 accent blue
    }
}

class ModernMenu {
public:
    ModernMenu(HINSTANCE hInst);
    ~ModernMenu();

    void AddItem(UINT id, const std::wstring& text, bool checked = false, bool enabled = true);
    ModernMenu* AddSubMenu(const std::wstring& text); // Returns child menu object
    void AddSeparator();
    void Clear();

    UINT Show(HWND hParent, int x, int y, bool isSubMenu = false);

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    void RegisterWindowClass();
    void CalculateLayout(HDC hdc);
    void OnPaint(HDC hdc);
    void OnMouseMove(int x, int y);
    void OnClick(int x, int y);
    
    void OpenSubMenu(int index);
    void CloseSubMenu();
    void RunModalLoop();
    void EnableAcrylic(HWND hwnd);

    HINSTANCE m_hInst;
    HWND m_hWnd;
    HWND m_hParent;
    std::vector<ModernMenuItem> m_items;
    int m_width;
    int m_height;
    int m_hoverIndex;
    UINT m_selectedIndex;
    bool m_isRunning;
    int m_itemHeight;
    int m_separatorHeight;
    HFONT m_hFont;
    bool m_isSubMenu;
    ModernMenu* m_parentMenu;
    ModernMenu* m_activeSubMenu;
    
    // Timer for submenu opening
    int m_hoverTimerId;
    int m_lastHoveredForSub;
    
    // Theme state (detected from Windows)
    bool m_isDarkMode;

    // static ULONG_PTR s_gdiToken; // CR-005: Removed in favor of GdiPlusManager
    static bool s_classRegistered;
    static bool s_isShowing;
};

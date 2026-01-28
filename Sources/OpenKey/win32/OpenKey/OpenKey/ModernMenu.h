#pragma once

#include "stdafx.h"
#include <vector>
#include <string>
#include <gdiplus.h>

struct ModernMenuItem {
    UINT id;
    std::wstring text;
    bool isSeparator;
    bool isChecked;
    bool isEnabled;
    class ModernMenu* subMenu; // Pointer to child menu if any
    
    ModernMenuItem(UINT _id, const std::wstring& _text, bool _checked = false, bool _enabled = true)
        : id(_id), text(_text), isSeparator(false), isChecked(_checked), isEnabled(_enabled), subMenu(nullptr) {}
        
    static ModernMenuItem Separator() {
        ModernMenuItem item(0, L"");
        item.isSeparator = true;
        return item;
    }
};

#define CORNER_RADIUS 8
#define PADDING_X 16
#define PADDING_Y 8
#define ICON_WIDTH 24

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

    static ULONG_PTR s_gdiToken;
    static bool s_classRegistered;
    static bool s_isShowing;
};

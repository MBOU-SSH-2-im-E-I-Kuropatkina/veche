#include "graphics.h"
#include <windows.h>
#include <iostream>

namespace veche { namespace graphics {

static HWND     g_hwnd    = nullptr;
static HDC      g_hdc     = nullptr;
static int      g_width   = 0;
static int      g_height  = 0;
static COLORREF g_color   = RGB(255, 255, 255);
static bool     g_classReg = false;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CLOSE:   DestroyWindow(hwnd); return 0;
        case WM_DESTROY: PostQuitMessage(0);  return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            (void)dc;
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool openWindow(int width, int height, const std::string& title) {
    if (g_hwnd) return true;

    HINSTANCE hInst = GetModuleHandleW(nullptr);

    if (!g_classReg) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc   = WndProc;
        wc.hInstance     = hInst;
        wc.lpszClassName = L"VecheWindowClass";
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        if (!RegisterClassW(&wc)) {
            std::cerr << "[Вече] Не удалось зарегистрировать окно\n";
            return false;
        }
        g_classReg = true;
    }

    std::wstring wtitle;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
    if (wlen > 0) {
        wtitle.resize(wlen - 1);
        MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wtitle[0], wlen);
    }

    g_hwnd = CreateWindowExW(
        0, L"VecheWindowClass", wtitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width + 16, height + 39,
        nullptr, nullptr, hInst, nullptr);

    if (!g_hwnd) {
        std::cerr << "[Вече] Не удалось создать окно\n";
        return false;
    }

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    g_hdc = GetDC(g_hwnd);
    g_width = width;
    g_height = height;
    return true;
}

void setColor(int r, int g, int b) {
    if (r < 0) r = 0;
    if (r > 255) r = 255;
    if (g < 0) g = 0;
    if (g > 255) g = 255;
    if (b < 0) b = 0;
    if (b > 255) b = 255;
    g_color = RGB(r, g, b);
}

void clear() {
    if (!g_hdc) return;
    RECT r{0, 0, g_width, g_height};
    HBRUSH br = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(g_hdc, &r, br);
    DeleteObject(br);
}

void drawPoint(int x, int y) {
    if (!g_hdc) return;
    SetPixel(g_hdc, x, y, g_color);
}

void drawLine(int x1, int y1, int x2, int y2) {
    if (!g_hdc) return;
    HPEN pen = CreatePen(PS_SOLID, 1, g_color);
    HPEN old = (HPEN)SelectObject(g_hdc, pen);
    MoveToEx(g_hdc, x1, y1, nullptr);
    LineTo(g_hdc, x2, y2);
    SelectObject(g_hdc, old);
    DeleteObject(pen);
}

void drawRect(int x, int y, int w, int h) {
    if (!g_hdc) return;
    HPEN pen = CreatePen(PS_SOLID, 1, g_color);
    HPEN oldPen = (HPEN)SelectObject(g_hdc, pen);
    HBRUSH oldBr = (HBRUSH)SelectObject(g_hdc, GetStockObject(NULL_BRUSH));
    Rectangle(g_hdc, x, y, x + w, y + h);
    SelectObject(g_hdc, oldPen);
    SelectObject(g_hdc, oldBr);
    DeleteObject(pen);
}

void pumpMessages() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) { g_hwnd = nullptr; return; }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void sleepMs(int ms) {
    DWORD end = GetTickCount() + (DWORD)ms;
    while (GetTickCount() < end) {
        pumpMessages();
        Sleep(5);
    }
}

void closeWindow() {
    if (g_hwnd) {
        if (g_hdc) { ReleaseDC(g_hwnd, g_hdc); g_hdc = nullptr; }
        DestroyWindow(g_hwnd);
        g_hwnd = nullptr;
    }
    pumpMessages();
}

}} // namespace
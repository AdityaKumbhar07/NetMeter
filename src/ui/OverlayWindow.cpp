#include "OverlayWindow.h"
#include "SettingsGUI.h"
#include "DisplayState.h"
#include "Renderer.h"
#include "../config/Settings.h"
#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_RELOAD 1002

namespace OverlayWindow {

static HWND g_overlayHwnd = nullptr;
static HWND g_taskbarHwnd = nullptr;
static const wchar_t *OVERLAY_CLASS = L"NetMeterOverlay";
static bool g_classRegistered = false;
static HFONT g_font = nullptr;
static int g_dpi = 96;

NOTIFYICONDATAW g_nid = {};

void AddTrayIcon(HWND hwnd) {
  g_nid.cbSize = sizeof(g_nid);
  g_nid.hWnd = hwnd;
  g_nid.uID = 1;
  g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  g_nid.uCallbackMessage = WM_TRAYICON;
  g_nid.hIcon = ExtractIconW(GetModuleHandleW(nullptr), L"NetMeter.exe", 0);
  if (!g_nid.hIcon) g_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
  wcscpy_s(g_nid.szTip, L"NetMeter");
  Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon() {
  Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

void ShowContextMenu(HWND hwnd) {
  POINT pt;
  GetCursorPos(&pt);
  HMENU hMenu = CreatePopupMenu();
  InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TRAY_RELOAD, L"Settings");
  InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
  InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, L"Exit");
  SetForegroundWindow(hwnd);
  TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
  DestroyMenu(hMenu);
}

int Scale(int value) { return MulDiv(value, g_dpi, 96); }

void UpdateDpi() {
  HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
  if (hUser32) {
    typedef UINT(WINAPI * GetDpiForWindow_t)(HWND);
    auto pGetDpiForWindow = (GetDpiForWindow_t)GetProcAddress(hUser32, "GetDpiForWindow");
    if (pGetDpiForWindow && g_taskbarHwnd) {
      g_dpi = pGetDpiForWindow(g_taskbarHwnd);
      return;
    }
  }
  HDC hdc = GetDC(nullptr);
  if (hdc) {
    g_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(nullptr, hdc);
  }
}

void CreateOverlayFont() {
  if (g_font) { DeleteObject(g_font); g_font = nullptr; }
  int fontSize = g_settings.fontSize > 0 ? g_settings.fontSize : 12;
  int fontHeight = -MulDiv(fontSize, g_dpi, 72);
  int weight = (g_settings.fontStyle & 1) ? FW_BOLD : FW_NORMAL;
  BOOL italic = (g_settings.fontStyle & 2) ? TRUE : FALSE;
  g_font = CreateFontW(fontHeight, 0, 0, 0, weight, italic, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, g_settings.fontFamily.c_str());
}

COLORREF ParseColor(const std::wstring &c, COLORREF defaultColor) {
  if (c.empty()) return defaultColor;
  if (c[0] == L'#') {
    unsigned long val = wcstoul(c.c_str() + 1, nullptr, 16);
    return RGB((val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
  }
  if (_wcsicmp(c.c_str(), L"White") == 0) return RGB(255, 255, 255);
  if (_wcsicmp(c.c_str(), L"Black") == 0) return RGB(0, 0, 0);
  if (_wcsicmp(c.c_str(), L"Red") == 0) return RGB(255, 0, 0);
  if (_wcsicmp(c.c_str(), L"Green") == 0) return RGB(0, 255, 0);
  if (_wcsicmp(c.c_str(), L"Blue") == 0) return RGB(0, 0, 255);
  if (_wcsicmp(c.c_str(), L"Yellow") == 0) return RGB(255, 255, 0);
  if (_wcsicmp(c.c_str(), L"Gray") == 0) return RGB(128, 128, 128);
  return defaultColor;
}

COLORREF GetTextColor(int type) {
  DWORD useLightTheme = 1;
  DWORD dataSize = sizeof(useLightTheme);
  RegGetValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"SystemUsesLightTheme", RRF_RT_REG_DWORD, nullptr, &useLightTheme, &dataSize);
  COLORREF defaultColor = useLightTheme ? RGB(0, 0, 0) : RGB(255, 255, 255);
  
  if (type == 1) return ParseColor(g_settings.textColorDl, defaultColor);
  if (type == 2) return ParseColor(g_settings.textColorUl, defaultColor);
  return defaultColor;
}

COLORREF GetBackgroundColor(bool &isTransparent) {
  isTransparent = false;
  DWORD useLightTheme = 1;
  DWORD dataSize = sizeof(useLightTheme);
  RegGetValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"SystemUsesLightTheme", RRF_RT_REG_DWORD, nullptr, &useLightTheme, &dataSize);
  COLORREF defaultColor = useLightTheme ? RGB(243, 243, 243) : RGB(32, 32, 32);

  if (_wcsicmp(g_settings.backgroundColor.c_str(), L"Transparent") == 0 || g_settings.bgOpacity == 0) {
    isTransparent = true;
    return defaultColor; // Colorkey
  }
  return ParseColor(g_settings.backgroundColor, defaultColor);
}

bool IsTaskbarCoveredByFullScreenApp() {
  HWND hWndForeground = GetForegroundWindow();
  if (!hWndForeground || hWndForeground == GetDesktopWindow() || hWndForeground == g_taskbarHwnd) return false;
  WCHAR className[256];
  if (GetClassNameW(hWndForeground, className, 256)) {
    if (wcscmp(className, L"WorkerW") == 0 || wcscmp(className, L"Progman") == 0) return false;
  }
  HMONITOR hTaskbarMonitor = MonitorFromWindow(g_taskbarHwnd, MONITOR_DEFAULTTONEAREST);
  HMONITOR hForegroundMonitor = MonitorFromWindow(hWndForeground, MONITOR_DEFAULTTONEAREST);
  if (hTaskbarMonitor != hForegroundMonitor) return false;
  MONITORINFO mi = {sizeof(mi)};
  if (!GetMonitorInfoW(hTaskbarMonitor, &mi)) return false;
  RECT rcApp;
  GetWindowRect(hWndForeground, &rcApp);
  if (rcApp.left <= mi.rcMonitor.left && rcApp.top <= mi.rcMonitor.top && rcApp.right >= mi.rcMonitor.right && rcApp.bottom >= mi.rcMonitor.bottom) {
    return true;
  }
  return false;
}

void PositionOverlay() {
  if (!g_overlayHwnd) return;
  if (!IsWindow(g_taskbarHwnd)) {
    g_taskbarHwnd = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!g_taskbarHwnd) return;
    SetWindowLongPtrW(g_overlayHwnd, GWLP_HWNDPARENT, (LONG_PTR)g_taskbarHwnd);
  }
  UpdateDpi();

  RECT taskbarRect;
  GetWindowRect(g_taskbarHwnd, &taskbarRect);
  int taskbarHeight = taskbarRect.bottom - taskbarRect.top;

  HDC hdc = GetDC(g_overlayHwnd);
  HFONT oldFont = (HFONT)SelectObject(hdc, g_font);
  int totalWidth = 0;
  for (const auto& el : DisplayState::g_lastDisplayText) {
    SIZE sz;
    GetTextExtentPoint32W(hdc, el.text.c_str(), (int)el.text.length(), &sz);
    totalWidth += sz.cx;
  }
  if (totalWidth == 0) totalWidth = Scale(60);
  SelectObject(hdc, oldFont);
  ReleaseDC(g_overlayHwnd, hdc);

  int overlayWidth = totalWidth + Scale(16);
  int overlayHeight = taskbarHeight;

  HWND hNotify = FindWindowExW(g_taskbarHwnd, nullptr, L"TrayNotifyWnd", nullptr);
  int xPos;
  if (hNotify) {
    RECT notifyRect;
    GetWindowRect(hNotify, &notifyRect);
    xPos = notifyRect.left - overlayWidth - Scale(10);
  } else {
    xPos = taskbarRect.right - overlayWidth - Scale(100);
  }
  if (xPos < taskbarRect.left) xPos = taskbarRect.left;

  if (!IsWindowVisible(g_taskbarHwnd) || IsTaskbarCoveredByFullScreenApp()) {
    ShowWindow(g_overlayHwnd, SW_HIDE);
  } else {
    ShowWindow(g_overlayHwnd, SW_SHOWNA);
  }
  SetWindowPos(g_overlayHwnd, nullptr, xPos, taskbarRect.top, overlayWidth, overlayHeight, SWP_NOACTIVATE | SWP_NOZORDER);
}

void PaintOverlay(HWND hwnd) {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd, &ps);
  RECT clientRect;
  GetClientRect(hwnd, &clientRect);

  HDC memDC = CreateCompatibleDC(hdc);
  HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
  HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

  bool isTransparent = false;
  COLORREF bgColor = GetBackgroundColor(isTransparent);

  if (isTransparent) {
    SetLayeredWindowAttributes(hwnd, bgColor, 255, LWA_COLORKEY | LWA_ALPHA);
  } else {
    SetLayeredWindowAttributes(hwnd, 0, (BYTE)g_settings.bgOpacity, LWA_ALPHA);
  }

  HBRUSH bgBrush = CreateSolidBrush(bgColor);
  FillRect(memDC, &clientRect, bgBrush);
  DeleteObject(bgBrush);

  HFONT oldFont = (HFONT)SelectObject(memDC, g_font);
  SetBkMode(memDC, TRANSPARENT);

  int totalWidth = 0;
  for (const auto& el : DisplayState::g_lastDisplayText) {
    SIZE sz;
    GetTextExtentPoint32W(memDC, el.text.c_str(), (int)el.text.length(), &sz);
    totalWidth += sz.cx;
  }
  
  int xOffset = (clientRect.right - totalWidth) / 2;

  for (const auto& el : DisplayState::g_lastDisplayText) {
    SIZE sz;
    GetTextExtentPoint32W(memDC, el.text.c_str(), (int)el.text.length(), &sz);
    SetTextColor(memDC, GetTextColor(el.type));
    RECT r = { xOffset, 0, xOffset + sz.cx, clientRect.bottom };
    DrawTextW(memDC, el.text.c_str(), (int)el.text.length(), &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    xOffset += sz.cx;
  }

  SelectObject(memDC, oldFont);
  BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);
  SelectObject(memDC, oldBitmap);
  DeleteObject(memBitmap);
  DeleteDC(memDC);
  EndPaint(hwnd, &ps);
}

void Reposition() {
  if (g_overlayHwnd) {
    PositionOverlay();
    InvalidateRect(g_overlayHwnd, nullptr, TRUE);
  }
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_PAINT: PaintOverlay(hwnd); return 0;
  case WM_ERASEBKGND: return 1;
  case WM_DISPLAYCHANGE:
  case WM_DPICHANGED:
  case WM_THEMECHANGED:
    UpdateDpi(); CreateOverlayFont(); PositionOverlay(); InvalidateRect(hwnd, nullptr, TRUE); return 0;
  case WM_TIMER:
    if (wParam == 1) {
      auto text = Renderer::BuildSpeedText();
      bool changed = text.size() != DisplayState::g_lastDisplayText.size();
      if (!changed) {
        for (size_t i=0; i<text.size(); i++) {
          if (text[i].text != DisplayState::g_lastDisplayText[i].text || text[i].type != DisplayState::g_lastDisplayText[i].type) {
            changed = true; break;
          }
        }
      }
      if (changed) {
        DisplayState::g_lastDisplayText = text;
        PositionOverlay();
        InvalidateRect(hwnd, nullptr, FALSE);
      }
    } else if (wParam == 2) {
      PositionOverlay();
    }
    return 0;
  case WM_NCHITTEST: return HTTRANSPARENT;
  case WM_TRAYICON:
    if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) ShowContextMenu(hwnd);
    return 0;
  case WM_COMMAND:
    if (LOWORD(wParam) == ID_TRAY_EXIT) {
      PostQuitMessage(0);
    } else if (LOWORD(wParam) == ID_TRAY_RELOAD) {
      if (lParam == 1) {
        LoadSettings(); UpdateDpi(); CreateOverlayFont(); DisplayState::Clear(); Reposition();
      } else {
        SettingsGUI::Show();
      }
    }
    return 0;
  case WM_DESTROY: PostQuitMessage(0); return 0;
  default: return DefWindowProcW(hwnd, msg, wParam, lParam);
  }
}

bool Create() {
  g_taskbarHwnd = FindWindowW(L"Shell_TrayWnd", nullptr);
  if (!g_taskbarHwnd) return false;
  UpdateDpi();

  if (!g_classRegistered) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = OVERLAY_CLASS;
    RegisterClassExW(&wc);
    g_classRegistered = true;
  }

  CreateOverlayFont();
  g_overlayHwnd = CreateWindowExW(
      WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED, OVERLAY_CLASS, L"NetMeterOverlay", WS_POPUP | WS_VISIBLE, 0, 0, Scale(200), Scale(40),
      g_taskbarHwnd, nullptr, GetModuleHandleW(nullptr), nullptr);

  if (!g_overlayHwnd) return false;
  SetLayeredWindowAttributes(g_overlayHwnd, 0, 255, LWA_ALPHA);
  PositionOverlay();
  SetTimer(g_overlayHwnd, 1, g_settings.refreshInterval, nullptr);
  SetTimer(g_overlayHwnd, 2, 5000, nullptr);
  AddTrayIcon(g_overlayHwnd);
  return true;
}

void Destroy() {
  if (g_overlayHwnd) {
    RemoveTrayIcon();
    KillTimer(g_overlayHwnd, 1);
    KillTimer(g_overlayHwnd, 2);
    DestroyWindow(g_overlayHwnd);
    g_overlayHwnd = nullptr;
  }
  if (g_font) { DeleteObject(g_font); g_font = nullptr; }
  g_taskbarHwnd = nullptr;
}
} // namespace OverlayWindow

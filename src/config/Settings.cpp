#include "Settings.h"
#include <shlwapi.h>

Settings g_settings;

std::wstring GetIniPath() {
  WCHAR path[MAX_PATH];
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  std::wstring p(path);
  size_t pos = p.find_last_of(L"\\/");
  return p.substr(0, pos) + L"\\settings.ini";
}

bool GetStartupRegistry() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR path[MAX_PATH];
        DWORD size = sizeof(path);
        if (RegQueryValueExW(hKey, L"NetMeter", nullptr, nullptr, (LPBYTE)path, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
        RegCloseKey(hKey);
    }
    return false;
}

void SetStartupRegistry(bool enable) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            WCHAR path[MAX_PATH];
            GetModuleFileNameW(nullptr, path, MAX_PATH);
            std::wstring quotePath = L"\"" + std::wstring(path) + L"\"";
            RegSetValueExW(hKey, L"NetMeter", 0, REG_SZ, (const BYTE*)quotePath.c_str(), (quotePath.length() + 1) * sizeof(WCHAR));
        } else {
            RegDeleteValueW(hKey, L"NetMeter");
        }
        RegCloseKey(hKey);
    }
}

void LoadSettings() {
  std::wstring ini = GetIniPath();
  g_settings.runOnStartup = GetStartupRegistry();
  g_settings.refreshInterval = GetPrivateProfileIntW(L"Settings", L"RefreshInterval", 1000, ini.c_str());
  if (g_settings.refreshInterval < 100) g_settings.refreshInterval = 100;

  g_settings.displayMode = GetPrivateProfileIntW(L"Settings", L"DisplayMode", 0, ini.c_str());
  g_settings.orderUlFirst = GetPrivateProfileIntW(L"Settings", L"OrderUlFirst", 0, ini.c_str()) != 0;
  g_settings.prefixMode = GetPrivateProfileIntW(L"Settings", L"PrefixMode", 0, ini.c_str());
  g_settings.unitMode = GetPrivateProfileIntW(L"Settings", L"UnitMode", 0, ini.c_str());
  g_settings.useCompactMode = GetPrivateProfileIntW(L"Settings", L"UseCompactMode", 0, ini.c_str()) != 0;

  WCHAR buf[256];
  GetPrivateProfileStringW(L"Settings", L"DlPrefix", L"D: ", buf, 256, ini.c_str());
  g_settings.dlPrefix = buf;
  GetPrivateProfileStringW(L"Settings", L"UlPrefix", L"U: ", buf, 256, ini.c_str());
  g_settings.ulPrefix = buf;

  g_settings.fontSize = GetPrivateProfileIntW(L"Settings", L"FontSize", 0, ini.c_str());
  g_settings.fontStyle = GetPrivateProfileIntW(L"Settings", L"FontStyle", 0, ini.c_str());

  GetPrivateProfileStringW(L"Settings", L"FontFamily", L"Segoe UI", buf, 256, ini.c_str());
  g_settings.fontFamily = buf;

  GetPrivateProfileStringW(L"Settings", L"TextColorDl", L"White", buf, 256, ini.c_str());
  g_settings.textColorDl = buf;
  GetPrivateProfileStringW(L"Settings", L"TextColorUl", L"White", buf, 256, ini.c_str());
  g_settings.textColorUl = buf;

  GetPrivateProfileStringW(L"Settings", L"BackgroundColor", L"Transparent", buf, 256, ini.c_str());
  g_settings.backgroundColor = buf;

  g_settings.bgOpacity = GetPrivateProfileIntW(L"Settings", L"BgOpacity", 255, ini.c_str());

  GetPrivateProfileStringW(L"Settings", L"NetworkAdapterName", L"", buf, 256, ini.c_str());
  g_settings.networkAdapterName = buf;
}

void SaveConfig() {
    SetStartupRegistry(g_settings.runOnStartup);
    std::wstring ini = GetIniPath();
    WCHAR buf[256];
    
    swprintf_s(buf, L"%d", g_settings.refreshInterval);
    WritePrivateProfileStringW(L"Settings", L"RefreshInterval", buf, ini.c_str());

    WritePrivateProfileStringW(L"Settings", L"NetworkAdapterName", g_settings.networkAdapterName.c_str(), ini.c_str());

    swprintf_s(buf, L"%d", g_settings.unitMode);
    WritePrivateProfileStringW(L"Settings", L"UnitMode", buf, ini.c_str());

    WritePrivateProfileStringW(L"Settings", L"TextColorDl", g_settings.textColorDl.c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Settings", L"TextColorUl", g_settings.textColorUl.c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Settings", L"BackgroundColor", g_settings.backgroundColor.c_str(), ini.c_str());
    
    swprintf_s(buf, L"%d", g_settings.bgOpacity);
    WritePrivateProfileStringW(L"Settings", L"BgOpacity", buf, ini.c_str());

    swprintf_s(buf, L"%d", g_settings.displayMode);
    WritePrivateProfileStringW(L"Settings", L"DisplayMode", buf, ini.c_str());

    swprintf_s(buf, L"%d", g_settings.orderUlFirst ? 1 : 0);
    WritePrivateProfileStringW(L"Settings", L"OrderUlFirst", buf, ini.c_str());

    swprintf_s(buf, L"%d", g_settings.prefixMode);
    WritePrivateProfileStringW(L"Settings", L"PrefixMode", buf, ini.c_str());

    WritePrivateProfileStringW(L"Settings", L"DlPrefix", g_settings.dlPrefix.c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Settings", L"UlPrefix", g_settings.ulPrefix.c_str(), ini.c_str());

    WritePrivateProfileStringW(L"Settings", L"UseCompactMode", g_settings.useCompactMode ? L"1" : L"0", ini.c_str());

    WritePrivateProfileStringW(L"Settings", L"FontFamily", g_settings.fontFamily.c_str(), ini.c_str());
    swprintf_s(buf, L"%d", g_settings.fontSize);
    WritePrivateProfileStringW(L"Settings", L"FontSize", buf, ini.c_str());
    swprintf_s(buf, L"%d", g_settings.fontStyle);
    WritePrivateProfileStringW(L"Settings", L"FontStyle", buf, ini.c_str());
}

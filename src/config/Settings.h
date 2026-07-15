#pragma once
#include <string>
#include <windows.h>

struct Settings {
  int refreshInterval;
  int displayMode; // 0=Both, 1=DL Only, 2=UL Only
  bool orderUlFirst; // false=DL then UL, true=UL then DL
  int prefixMode; // 0=Arrows, 1=Letters, 2=None
  int unitMode; // 0=Auto B/s, 1=Auto bps, 2=KB/s, 3=MB/s, 4=GB/s, 5=Kbps, 6=Mbps, 7=Gbps
  bool useCompactMode;
  bool runOnStartup;
  
  std::wstring dlPrefix;
  std::wstring ulPrefix;

  int fontSize;
  int fontStyle;
  std::wstring fontFamily;

  std::wstring textColorDl;
  std::wstring textColorUl;
  std::wstring backgroundColor;
  int bgOpacity; // 0-255

  std::wstring networkAdapterName;
};

extern Settings g_settings;

std::wstring GetIniPath();
bool GetStartupRegistry();
void SetStartupRegistry(bool enable);
void LoadSettings();
void SaveConfig();

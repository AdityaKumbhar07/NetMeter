#include "Renderer.h"
#include "../config/Settings.h"
#include "../network/NetMonitor.h"
#include <windows.h>

namespace Renderer {

std::wstring FormatSpeed(double bytesPerSec, std::wstring &unitOut) {
  if (bytesPerSec < 0.0) {
    unitOut = L"";
    return L"--";
  }

  const double KB = 1024.0;
  const double MB = 1024.0 * 1024.0;
  const double GB = 1024.0 * 1024.0 * 1024.0;

  double bitsPerSec = bytesPerSec * 8.0;
  const double Kb = 1000.0;
  const double Mb = 1000.0 * 1000.0;
  const double Gb = 1000.0 * 1000.0 * 1000.0;

  WCHAR buf[64];
  
  switch(g_settings.unitMode) {
    case 0: // Auto B/s
      if (bytesPerSec < KB) { unitOut = L"B/s"; swprintf_s(buf, L"%.0f", bytesPerSec); }
      else if (bytesPerSec < MB) { unitOut = L"KB/s"; swprintf_s(buf, L"%.1f", bytesPerSec / KB); }
      else if (bytesPerSec < GB) { unitOut = L"MB/s"; swprintf_s(buf, L"%.2f", bytesPerSec / MB); }
      else { unitOut = L"GB/s"; swprintf_s(buf, L"%.2f", bytesPerSec / GB); }
      break;
    case 1: // Auto bps
      if (bitsPerSec < Kb) { unitOut = L"bps"; swprintf_s(buf, L"%.0f", bitsPerSec); }
      else if (bitsPerSec < Mb) { unitOut = L"Kbps"; swprintf_s(buf, L"%.1f", bitsPerSec / Kb); }
      else if (bitsPerSec < Gb) { unitOut = L"Mbps"; swprintf_s(buf, L"%.2f", bitsPerSec / Mb); }
      else { unitOut = L"Gbps"; swprintf_s(buf, L"%.2f", bitsPerSec / Gb); }
      break;
    case 2: unitOut = L"KB/s"; swprintf_s(buf, L"%.1f", bytesPerSec / KB); break;
    case 3: unitOut = L"MB/s"; swprintf_s(buf, L"%.2f", bytesPerSec / MB); break;
    case 4: unitOut = L"GB/s"; swprintf_s(buf, L"%.2f", bytesPerSec / GB); break;
    case 5: unitOut = L"Kbps"; swprintf_s(buf, L"%.1f", bitsPerSec / Kb); break;
    case 6: unitOut = L"Mbps"; swprintf_s(buf, L"%.2f", bitsPerSec / Mb); break;
    case 7: unitOut = L"Gbps"; swprintf_s(buf, L"%.2f", bitsPerSec / Gb); break;
  }
  return buf;
}

std::vector<DrawElement> BuildSpeedText() {
  double dl = NetMonitor::GetDownloadSpeed();
  double ul = NetMonitor::GetUploadSpeed();
  
  std::wstring dlUnit, ulUnit;
  std::wstring dlStr = FormatSpeed(dl, dlUnit);
  std::wstring ulStr = FormatSpeed(ul, ulUnit);

  std::wstring dlPrefix = L"";
  std::wstring ulPrefix = L"";
  if (g_settings.prefixMode == 0) {
    dlPrefix = L"\x2193 ";
    ulPrefix = L"\x2191 ";
  } else if (g_settings.prefixMode == 1) {
    dlPrefix = g_settings.dlPrefix;
    ulPrefix = g_settings.ulPrefix;
  }

  std::vector<DrawElement> elements;
  
  auto addDl = [&]() {
    std::wstring text = dlPrefix + dlStr;
    if (!g_settings.useCompactMode) text += L" " + dlUnit;
    elements.push_back({text, 1});
  };
  
  auto addUl = [&]() {
    std::wstring text = ulPrefix + ulStr;
    if (!g_settings.useCompactMode) text += L" " + ulUnit;
    elements.push_back({text, 2});
  };

  if (g_settings.displayMode == 0) {
    if (!g_settings.orderUlFirst) {
      addDl();
      elements.push_back({L"  ", 0});
      addUl();
    } else {
      addUl();
      elements.push_back({L"  ", 0});
      addDl();
    }
  } else if (g_settings.displayMode == 1) {
    addDl();
  } else if (g_settings.displayMode == 2) {
    addUl();
  }

  if (g_settings.useCompactMode) {
    std::wstring unit = dlUnit >= ulUnit ? dlUnit : ulUnit;
    if (dl >= 0 || ul >= 0) {
      if (unit.size() > 0 && (unit[0] == L'K' || unit[0] == L'M' || unit[0] == L'G')) {
         unit = unit.substr(0, 1);
      }
      elements.push_back({L" " + unit, 0});
    }
  }
  
  return elements;
}

} // namespace Renderer

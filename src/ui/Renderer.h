#pragma once
#include <string>
#include <vector>

namespace Renderer {
  struct DrawElement {
    std::wstring text;
    int type; // 0=Normal, 1=DL, 2=UL
  };

  std::wstring FormatSpeed(double bytesPerSec, std::wstring &unitOut);
  std::vector<DrawElement> BuildSpeedText();
}

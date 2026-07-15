#pragma once
#include "Renderer.h"
#include <vector>

namespace DisplayState {
  extern std::vector<Renderer::DrawElement> g_lastDisplayText;
  void Clear();
}

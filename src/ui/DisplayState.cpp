#include "DisplayState.h"

namespace DisplayState {
std::vector<Renderer::DrawElement> g_lastDisplayText;
void Clear() { g_lastDisplayText.clear(); }
}

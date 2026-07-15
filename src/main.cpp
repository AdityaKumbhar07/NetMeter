#include <windows.h>
#include "config/Settings.h"
#include "network/NetMonitor.h"
#include "ui/OverlayWindow.h"
#include "ui/DisplayState.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"NetMeterStandaloneMutex");
  if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;
  LoadSettings();
  NetMonitor::Start();
  if (!OverlayWindow::Create()) { NetMonitor::Stop(); return 1; }
  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
  OverlayWindow::Destroy();
  DisplayState::Clear();
  NetMonitor::Stop();
  ReleaseMutex(hMutex); CloseHandle(hMutex);
  return 0;
}

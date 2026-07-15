#pragma once
#include <string>
#include <windows.h>

namespace NetMonitor {
  void Start();
  void Stop();
  double GetDownloadSpeed();
  double GetUploadSpeed();
}

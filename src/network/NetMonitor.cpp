#include "NetMonitor.h"
#include "../config/Settings.h"
#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <thread>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")

namespace NetMonitor {

struct AdapterStats {
  DWORD inOctets;
  DWORD outOctets;
  ULONGLONG timestamp; // GetTickCount64
};

static std::map<DWORD, AdapterStats> g_prevStats;
static std::atomic<bool> g_stopPolling{false};
static std::thread g_pollingThread;
static std::mutex g_speedMutex;

static double g_downloadSpeed = 0.0;
static double g_uploadSpeed = 0.0;

bool MatchesAdapterFilter(const BYTE *descr, DWORD descrLen, const std::wstring &filter) {
  if (filter.empty()) {
    return true;
  }
  std::wstring descW;
  for (DWORD i = 0; i < descrLen; ++i) {
    descW += (wchar_t)descr[i];
  }
  std::transform(descW.begin(), descW.end(), descW.begin(), ::tolower);
  std::wstring filterLower = filter;
  std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
  return descW.find(filterLower) != std::wstring::npos;
}

void PollNetworkStats() {
  DWORD dwSize = 0;
  DWORD dwRetVal = GetIfTable(nullptr, &dwSize, FALSE);

  if (dwRetVal != ERROR_INSUFFICIENT_BUFFER) return;

  PMIB_IFTABLE pIfTable = (PMIB_IFTABLE)malloc(dwSize);
  if (!pIfTable) return;

  dwRetVal = GetIfTable(pIfTable, &dwSize, FALSE);
  if (dwRetVal != NO_ERROR) {
    free(pIfTable);
    return;
  }

  bool foundAdapter = false;
  ULONGLONG now = GetTickCount64();

  DWORD maxDiff = 0;
  double bestDlSpeed = 0.0;
  double bestUlSpeed = 0.0;

  std::map<DWORD, AdapterStats> newStats;

  for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
    MIB_IFROW &row = pIfTable->table[i];

    if (row.dwOperStatus != 5 && row.dwOperStatus != 4) continue;
    if (row.dwType != 6 && row.dwType != 71 && row.dwType != 23 && row.dwType != 144) continue;

    if (row.dwDescrLen > 0) {
      std::string desc((const char *)row.bDescr, row.dwDescrLen);
      std::transform(desc.begin(), desc.end(), desc.begin(), ::tolower);
      if (desc.find("loopback") != std::string::npos ||
          desc.find("pseudo") != std::string::npos ||
          desc.find("isatap") != std::string::npos ||
          desc.find("teredo") != std::string::npos) {
        continue;
      }
    }

    if (!MatchesAdapterFilter(row.bDescr, row.dwDescrLen, g_settings.networkAdapterName)) continue;

    foundAdapter = true;

    auto it = g_prevStats.find(row.dwIndex);
    if (it != g_prevStats.end()) {
      AdapterStats &prev = it->second;
      double elapsed = (double)(now - prev.timestamp) / 1000.0;
      if (elapsed > 0.0) {
        DWORD inDiff = row.dwInOctets - prev.inOctets;
        DWORD outDiff = row.dwOutOctets - prev.outOctets;

        if ((inDiff + outDiff) >= maxDiff) {
          maxDiff = inDiff + outDiff;
          bestDlSpeed = (double)inDiff / elapsed;
          bestUlSpeed = (double)outDiff / elapsed;
        }
      }
    }

    newStats[row.dwIndex] = {row.dwInOctets, row.dwOutOctets, now};
  }

  free(pIfTable);

  g_prevStats = std::move(newStats);

  std::lock_guard<std::mutex> lock(g_speedMutex);
  if (!foundAdapter) {
    g_downloadSpeed = -1.0; 
    g_uploadSpeed = -1.0;
  } else {
    g_downloadSpeed = bestDlSpeed;
    g_uploadSpeed = bestUlSpeed;
  }
}

void PollingThreadProc() {
  while (!g_stopPolling.load()) {
    PollNetworkStats();
    int remaining = g_settings.refreshInterval;
    while (remaining > 0 && !g_stopPolling.load()) {
      int sleepTime = (std::min)(remaining, 100);
      Sleep(sleepTime);
      remaining -= sleepTime;
    }
  }
}

void Start() {
  g_stopPolling.store(false);
  g_prevStats.clear();
  g_downloadSpeed = 0.0;
  g_uploadSpeed = 0.0;
  g_pollingThread = std::thread(PollingThreadProc);
}

void Stop() {
  g_stopPolling.store(true);
  if (g_pollingThread.joinable()) {
    g_pollingThread.join();
  }
}

double GetDownloadSpeed() {
  std::lock_guard<std::mutex> lock(g_speedMutex);
  return g_downloadSpeed;
}

double GetUploadSpeed() {
  std::lock_guard<std::mutex> lock(g_speedMutex);
  return g_uploadSpeed;
}

} // namespace NetMonitor

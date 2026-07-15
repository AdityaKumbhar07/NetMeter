# NetMeter

A standalone C++ Windows desktop application that displays live network download and upload speeds on the Windows taskbar.

## Overview

NetMeter runs as a single portable Windows executable (`NetMeter.exe`) with a single-instance mutex (`NetMeterStandaloneMutex`). It polls active network interface byte counters on a background thread, renders live throughput over the taskbar near the notification area, and provides a tray-accessible settings window built with Dear ImGui and Direct3D 11.

## Features

- Polls network adapter statistics using the Windows IP Helper API (`GetIfTable`) on a dedicated background thread.
- Computes bytes per second from `GetTickCount64()` timed deltas with 32-bit `DWORD` counter wraparound handling.
- Filters adapters by operational status, interface type (Ethernet, Wi-Fi, PPP, IEEE 1394), and description-based exclusion of loopback and virtual/tunnel adapters (`loopback`, `pseudo`, `isatap`, `teredo`), with an optional user-specified adapter-name substring filter.
- Displays the overlay as a click-through (`WS_EX_TRANSPARENT`, `HTTRANSPARENT`), layered (`WS_EX_LAYERED`) popup window anchored next to `TrayNotifyWnd` on `Shell_TrayWnd`.
- Automatically hides the overlay when the taskbar is hidden or covered by a fullscreen window on the same monitor.
- Uses double-buffered GDI rendering (`CreateCompatibleDC`, `CreateCompatibleBitmap`, `BitBlt`), per-window DPI scaling (`GetDpiForWindow`), and Windows light/dark theme detection (`SystemUsesLightTheme` registry key).
- Integrates a system tray icon and context menu (`Shell_NotifyIconW`) for opening settings or exiting.
- Supports configurable units (auto `B/s`, auto `bps`, or fixed `KB/s`, `MB/s`, `GB/s`, `Kbps`, `Mbps`, `Gbps`), compact formatting, custom prefixes, upload/download ordering, per-direction colors, background color/opacity, and font selection (`ChooseFontW`).
- Persists configuration in `settings.ini` with live hot-reload via `WM_COMMAND`.
- Provides an optional Windows startup toggle via the `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` registry key.
- Settings window rendered with Dear ImGui (v1.90.8) on a Direct3D 11 (`D3D11`) swap chain.

## Screenshots

_TODO: Add screenshots of the taskbar overlay and settings window._

## How it works

1. **Startup and configuration (`src/main.cpp`, `src/config/Settings.cpp`)**: Acquires a named mutex, reads `settings.ini` from the executable directory via `GetPrivateProfileIntW` / `GetPrivateProfileStringW`, and checks `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` for startup status.
2. **Background network polling (`src/network/NetMonitor.cpp`)**: A `std::thread` polls `GetIfTable` every `RefreshInterval` ms (minimum `100` ms, default `1000` ms). It retains per-interface `{inOctets, outOctets, timestamp}` state in a map, computes download and upload rates across the measured `GetTickCount64()` interval, selects the active filtered adapter with the largest byte delta, and stores the latest speeds under a `std::mutex`.
3. **Overlay formatting and rendering (`src/ui/Renderer.cpp`, `src/ui/OverlayWindow.cpp`)**: A Win32 `WM_TIMER` triggers text formatting; if the output differs from `DisplayState::g_lastDisplayText`, the window repositions relative to `TrayNotifyWnd` and repaints via double-buffered GDI (`DrawTextW` + `BitBlt`). A secondary 5-second timer keeps position and fullscreen visibility synchronized.
4. **Settings interface (`src/ui/SettingsGUI.cpp`)**: Selecting **Settings** from the tray menu spawns a dedicated UI thread creating a Win32 window backed by a `D3D11CreateDeviceAndSwapChain` device and Dear ImGui. Clicking **Save & Apply** writes `settings.ini`, updates the startup registry entry, and sends `WM_COMMAND` (`1002`) to `NetMeterOverlay` to reload settings immediately.

## Build instructions

### Prerequisites

- Windows 10/11 SDK headers and import libraries (bundled with MinGW-w64 / LLVM Clang toolchain).
- `clang++.exe` compatible with Windows targets and static C++ runtime linking.
- `rcedit.exe` (placed in the repository root or on `PATH`) to embed `assets/app.ico` into `NetMeter.exe`.

### Compiler path

`build.bat` reads `CLANG_PATH` from the environment and defaults to:

```text
C:\Program Files\Windhawk\Compiler\bin\clang++.exe
```

If your `clang++.exe` is located elsewhere, set `CLANG_PATH` before running `build.bat`:

```bat
set "CLANG_PATH=C:\path\to\clang++.exe"
```

### Build command

Run from the repository root (`NetMeter_Repo`):

```bat
build.bat
```

Underlying compiler invocation executed by `build.bat`:

```bat
"%CLANG_PATH%" ^
 src\main.cpp ^
 src\utils\Logger.cpp ^
 src\config\Settings.cpp ^
 src\network\NetMonitor.cpp ^
 src\ui\Renderer.cpp ^
 src\ui\DisplayState.cpp ^
 src\ui\OverlayWindow.cpp ^
 src\ui\SettingsGUI.cpp ^
 third_party\imgui\imgui.cpp third_party\imgui\imgui_draw.cpp third_party\imgui\imgui_tables.cpp third_party\imgui\imgui_widgets.cpp ^
 third_party\imgui\backends\imgui_impl_win32.cpp third_party\imgui\backends\imgui_impl_dx11.cpp ^
 -o NetMeter.exe -O2 -DUNICODE -D_UNICODE -mwindows -static -Wl,-s ^
 -I third_party\imgui -I third_party\imgui\backends ^
 -liphlpapi -lversion -lgdi32 -luser32 -lshell32 -lcomctl32 -lcomdlg32 -lws2_32 -ld3d11 -ld3dcompiler -ldwmapi -lshlwapi
```

## Usage

1. Run `NetMeter.exe`. The speed indicator appears on the taskbar to the left of the notification area, and a NetMeter icon is added to the system tray.
2. Right-click or left-click the system tray icon to open **Settings** or **Exit**.
3. Adjust refresh interval, adapter filter, display units, colors, layout, font, and startup behavior in the **Settings** window, then click **Save & Apply** to write `settings.ini` and update the running overlay immediately.

## Project structure

```text
NetMeter_Repo/
├── .gitignore
├── build.bat
├── LICENSE
├── README.md
├── assets/
│   └── app.ico
├── src/
│   ├── main.cpp
│   ├── config/
│   │   ├── Settings.cpp
│   │   └── Settings.h
│   ├── network/
│   │   ├── NetMonitor.cpp
│   │   └── NetMonitor.h
│   ├── ui/
│   │   ├── DisplayState.cpp
│   │   ├── DisplayState.h
│   │   ├── OverlayWindow.cpp
│   │   ├── OverlayWindow.h
│   │   ├── Renderer.cpp
│   │   ├── Renderer.h
│   │   ├── SettingsGUI.cpp
│   │   └── SettingsGUI.h
│   └── utils/
│       ├── Logger.cpp
│       └── Logger.h
└── third_party/
    └── imgui/
        ├── LICENSE.txt
        ├── imconfig.h
        ├── imgui.cpp
        ├── imgui.h
        ├── imgui_draw.cpp
        ├── imgui_internal.h
        ├── imgui_tables.cpp
        ├── imgui_widgets.cpp
        ├── imstb_rectpack.h
        ├── imstb_textedit.h
        ├── imstb_truetype.h
        └── backends/
            ├── imgui_impl_dx11.cpp
            ├── imgui_impl_dx11.h
            ├── imgui_impl_win32.cpp
            └── imgui_impl_win32.h
```

## License and third-party notices

- **NetMeter**: Licensed under the MIT License. See [LICENSE](LICENSE).
- **Third-party library**: Dear ImGui v1.90.8, MIT License, https://github.com/ocornut/imgui (license text included at [third_party/imgui/LICENSE.txt](third_party/imgui/LICENSE.txt)).

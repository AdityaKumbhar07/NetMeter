@echo off
setlocal
pushd "%~dp0"

if not defined CLANG_PATH set "CLANG_PATH=C:\Program Files\Windhawk\Compiler\bin\clang++.exe"
if not exist "%CLANG_PATH%" (
    echo [ERROR] C++ compiler not found at "%CLANG_PATH%".
    echo         Set CLANG_PATH to your clang++.exe path ^(for example, a MinGW-w64 Clang toolchain^) and retry.
    popd
    exit /b 1
)

echo Compiling NetMeter with Dear ImGui...
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
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation of NetMeter.exe failed.
    popd
    exit /b 1
)

if not exist "assets\app.ico" (
    echo [ERROR] Icon file not found at "assets\app.ico".
    popd
    exit /b 1
)

if exist "rcedit.exe" (
    echo Embedding custom icon into executable...
    rcedit.exe "NetMeter.exe" --set-icon "assets\app.ico"
    if %ERRORLEVEL% neq 0 (
        echo [ERROR] Failed to embed icon with rcedit.exe.
        popd
        exit /b 1
    )
) else (
    where rcedit >nul 2>nul
    if errorlevel 1 (
        echo [ERROR] rcedit.exe not found in repository root or on PATH. Icon embedding is required.
        popd
        exit /b 1
    )
    echo Embedding custom icon into executable...
    rcedit "NetMeter.exe" --set-icon "assets\app.ico"
    if %ERRORLEVEL% neq 0 (
        echo [ERROR] Failed to embed icon with rcedit.
        popd
        exit /b 1
    )
)

echo Compilation successful. Output: NetMeter.exe
popd

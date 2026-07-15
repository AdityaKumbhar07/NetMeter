#include "SettingsGUI.h"
#include "DisplayState.h"
#include "../config/Settings.h"
#include "../utils/Logger.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include <d3d11.h>
#include <thread>
#include <atomic>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace SettingsGUI {

static std::thread g_uiThread;
static std::atomic<bool> g_isOpen{false};

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
    return true;
}

void CleanupDeviceD3D() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            ID3D11Texture2D* pBackBuffer;
            g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
            pBackBuffer->Release();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Helpers for ImGui
ImVec4 HexToImVec4(const std::wstring& hex) {
    if(hex.empty() || hex[0] != L'#') return ImVec4(1,1,1,1);
    unsigned long val = wcstoul(hex.c_str() + 1, nullptr, 16);
    return ImVec4(((val >> 16) & 0xFF) / 255.0f, ((val >> 8) & 0xFF) / 255.0f, (val & 0xFF) / 255.0f, 1.0f);
}

std::wstring ImVec4ToHex(const ImVec4& c) {
    WCHAR buf[16];
    swprintf_s(buf, L"#%02X%02X%02X", (int)(c.x * 255.0f), (int)(c.y * 255.0f), (int)(c.z * 255.0f));
    return buf;
}

void SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.ItemSpacing = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]       = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_FrameBg]        = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
    colors[ImGuiCol_TitleBg]        = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TitleBgActive]  = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_CheckMark]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]     = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]= ImVec4(0.35f, 0.65f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]         = ImVec4(0.20f, 0.40f, 0.68f, 1.00f);
    colors[ImGuiCol_ButtonHovered]  = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]   = ImVec4(0.15f, 0.35f, 0.60f, 1.00f);
    colors[ImGuiCol_Header]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_HeaderHovered]  = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]   = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Tab]            = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_TabHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive]      = ImVec4(0.20f, 0.40f, 0.68f, 1.00f);
}

void UIThreadProc() {
    HICON hIcon = ExtractIconW(GetModuleHandleW(nullptr), L"NetMeter.exe", 0);
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), hIcon, nullptr, nullptr, nullptr, L"NetMeterImGui", hIcon };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"NetMeter Settings - Ultimate", WS_OVERLAPPEDWINDOW, 100, 100, 500, 600, nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        g_isOpen = false;
        return;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    SetupImGuiStyle();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Local copies of settings for UI editing
    int rInt = g_settings.refreshInterval;
    std::string adapterName = WideToStr(g_settings.networkAdapterName);
    bool runOnStartup = g_settings.runOnStartup;
    int unitMode = g_settings.unitMode;
    ImVec4 colDl = HexToImVec4(g_settings.textColorDl);
    ImVec4 colUl = HexToImVec4(g_settings.textColorUl);
    ImVec4 colBg = HexToImVec4(g_settings.backgroundColor);
    int bgOp = g_settings.bgOpacity;
    int dispMode = g_settings.displayMode;
    bool orderUlFirst = g_settings.orderUlFirst;
    int prefixMode = g_settings.prefixMode;
    std::string dlPref = WideToStr(g_settings.dlPrefix);
    std::string ulPref = WideToStr(g_settings.ulPrefix);
    bool compact = g_settings.useCompactMode;

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem("General")) {
                ImGui::Spacing();
                ImGui::SliderInt("Refresh Interval (ms)", &rInt, 100, 5000);
                
                char adapterBuf[256];
                strcpy_s(adapterBuf, adapterName.c_str());
                if (ImGui::InputText("Network Adapter", adapterBuf, 256)) adapterName = adapterBuf;
                ImGui::TextDisabled("Leave empty for all adapters");

                const char* units[] = {"Auto Bytes/s", "Auto Bits/s", "KB/s", "MB/s", "GB/s", "Kbps", "Mbps", "Gbps"};
                ImGui::Combo("Speed Unit", &unitMode, units, IM_ARRAYSIZE(units));
                
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                ImGui::Checkbox("Run on Windows Startup", &runOnStartup);

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Visuals")) {
                ImGui::Spacing();
                ImGui::ColorEdit3("Download Color", (float*)&colDl);
                ImGui::ColorEdit3("Upload Color", (float*)&colUl);
                ImGui::ColorEdit3("Background Color", (float*)&colBg);
                ImGui::SliderInt("Bg Opacity", &bgOp, 0, 255);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Layout")) {
                ImGui::Spacing();
                const char* modes[] = {"Both DL & UL", "Download Only", "Upload Only"};
                ImGui::Combo("Display Mode", &dispMode, modes, IM_ARRAYSIZE(modes));
                
                const char* orders[] = {"Download first", "Upload first"};
                int oIdx = orderUlFirst ? 1 : 0;
                if (ImGui::Combo("Order", &oIdx, orders, IM_ARRAYSIZE(orders))) orderUlFirst = (oIdx == 1);

                const char* prefs[] = {"Arrows (\xE2\x86\x93\xE2\x86\x91)", "Custom Text", "None"};
                ImGui::Combo("Prefix Style", &prefixMode, prefs, IM_ARRAYSIZE(prefs));

                if (prefixMode == 1) {
                    char dP[64], uP[64];
                    strcpy_s(dP, dlPref.c_str()); strcpy_s(uP, ulPref.c_str());
                    if (ImGui::InputText("Custom DL Prefix", dP, 64)) dlPref = dP;
                    if (ImGui::InputText("Custom UL Prefix", uP, 64)) ulPref = uP;
                }

                ImGui::Checkbox("Use Compact Units", &compact);

                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                std::string fontDesc = WideToStr(g_settings.fontFamily) + " (" + std::to_string(g_settings.fontSize > 0 ? g_settings.fontSize : 12) + "pt)";
                ImGui::Text("Current Font: %s", fontDesc.c_str());
                if (ImGui::Button("Choose Font...")) {
                    CHOOSEFONTW cf = {};
                    LOGFONTW lf = {};
                    wcscpy_s(lf.lfFaceName, g_settings.fontFamily.c_str());
                    lf.lfHeight = -MulDiv(g_settings.fontSize > 0 ? g_settings.fontSize : 12, 96, 72);
                    lf.lfWeight = (g_settings.fontStyle & 1) ? FW_BOLD : FW_NORMAL;
                    lf.lfItalic = (g_settings.fontStyle & 2) ? TRUE : FALSE;
                    cf.lStructSize = sizeof(cf);
                    cf.hwndOwner = hwnd;
                    cf.lpLogFont = &lf;
                    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
                    if (ChooseFontW(&cf)) {
                        g_settings.fontFamily = lf.lfFaceName;
                        g_settings.fontSize = -MulDiv(lf.lfHeight, 72, 96);
                        int fs = 0;
                        if (lf.lfWeight >= FW_BOLD) fs |= 1;
                        if (lf.lfItalic) fs |= 2;
                        g_settings.fontStyle = fs;
                    }
                }

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        if (ImGui::Button("Save & Apply", ImVec2(120, 30))) {
            g_settings.refreshInterval = rInt;
            g_settings.networkAdapterName = StrToWide(adapterName);
            g_settings.runOnStartup = runOnStartup;
            g_settings.unitMode = unitMode;
            g_settings.textColorDl = ImVec4ToHex(colDl);
            g_settings.textColorUl = ImVec4ToHex(colUl);
            g_settings.backgroundColor = ImVec4ToHex(colBg);
            g_settings.bgOpacity = bgOp;
            g_settings.displayMode = dispMode;
            g_settings.orderUlFirst = orderUlFirst;
            g_settings.prefixMode = prefixMode;
            g_settings.dlPrefix = StrToWide(dlPref);
            g_settings.ulPrefix = StrToWide(ulPref);
            g_settings.useCompactMode = compact;

            SaveConfig();
            HWND hOverlay = FindWindowW(L"NetMeterOverlay", nullptr);
            if (hOverlay) SendMessageW(hOverlay, WM_COMMAND, 1002, 1); // Trigger reload
            done = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 30))) {
            done = true;
        }

        ImGui::End();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    g_isOpen = false;
}

void Show() {
    if (g_isOpen.exchange(true)) return; // Already open
    if (g_uiThread.joinable()) g_uiThread.join();
    g_uiThread = std::thread(UIThreadProc);
}

} // namespace SettingsGUI

#include "Logger.h"
#include <windows.h>
#include <cstdio>
#include <cstdarg>

void Wh_Log(const wchar_t* format, ...) {
    FILE* fp = nullptr;
    if (_wfopen_s(&fp, L"debug.log", L"a") == 0 && fp) {
        va_list args;
        va_start(args, format);
        vfwprintf(fp, format, args);
        fwprintf(fp, L"\n");
        va_end(args);
        fclose(fp);
    }
}

std::string WideToStr(const std::wstring& w) {
    if (w.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &w[0], (int)w.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &w[0], (int)w.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring StrToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

#pragma once
#include <string>

void Wh_Log(const wchar_t* format, ...);
std::string WideToStr(const std::wstring& w);
std::wstring StrToWide(const std::string& str);

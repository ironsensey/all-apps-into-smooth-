#pragma once
#include <windows.h>
constexpr DWORD DWMWA_CLOAKED = 14;
HRESULT DwmGetWindowAttribute(HWND, DWORD, void*, DWORD);

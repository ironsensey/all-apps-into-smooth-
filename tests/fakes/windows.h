#pragma once
// Test-only Win32 declarations. Production builds use the real Windows SDK.
#include <cstddef>
#include <cstdint>
#define WINAPI
#define TRUE 1
#define FALSE 0
#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#define FAILED(hr) (static_cast<std::int32_t>(hr) < 0)
using BOOL = int;
using DWORD = std::uint32_t;
using LONG_PTR = std::intptr_t;
using HRESULT = std::int32_t;
using HWND = void*;
using HANDLE = void*;
using HMODULE = void*;
using PCWSTR = const wchar_t*;
using FARPROC = void(*)();
constexpr int VK_SHIFT = 16, VK_CONTROL = 17, VK_MENU = 18;
constexpr int GA_ROOT = 2, GW_OWNER = 4, GWL_STYLE = -16, GWL_EXSTYLE = -20;
constexpr LONG_PTR WS_CHILD = 0x40000000, WS_MINIMIZEBOX = 0x20000;
constexpr LONG_PTR WS_EX_TOOLWINDOW = 0x80, WS_EX_NOACTIVATE = 0x08000000;
constexpr int SW_HIDE = 0, SW_SHOWNORMAL = 1, SW_SHOW = 5;
constexpr int SW_MINIMIZE = 6, SW_RESTORE = 9;
constexpr DWORD PROCESS_QUERY_LIMITED_INFORMATION = 0x1000;
constexpr int CSTR_EQUAL = 2;
DWORD GetLastError();
void SetLastError(DWORD);
short GetKeyState(int);
BOOL IsWindow(HWND);
BOOL IsWindowVisible(HWND);
BOOL IsWindowEnabled(HWND);
BOOL IsIconic(HWND);
HWND GetAncestor(HWND, int);
HWND GetWindow(HWND, int);
LONG_PTR GetWindowLongPtrW(HWND, int);
HWND GetLastActivePopup(HWND);
DWORD GetWindowThreadProcessId(HWND, DWORD*);
DWORD GetCurrentThreadId();
DWORD GetCurrentProcessId();
HANDLE OpenProcess(DWORD, BOOL, DWORD);
BOOL QueryFullProcessImageNameW(HANDLE, DWORD, wchar_t*, DWORD*);
BOOL CloseHandle(HANDLE);
int CompareStringOrdinal(const wchar_t*, int, const wchar_t*, int, BOOL);
BOOL ShowWindow(HWND, int);
BOOL ShowWindowAsync(HWND, int);
void SwitchToThisWindow(HWND, BOOL);
HMODULE GetModuleHandleW(PCWSTR);
FARPROC GetProcAddress(HMODULE, const char*);

// ==WindhawkMod==
// @id              native-smooth-taskbar
// @name            Native Smooth Taskbar
// @description     Native Windows animations with configurable taskbar activation and optional nonblocking window requests.
// @version         0.1.0
// @author          Native Smooth Taskbar contributors
// @license         MIT
// @include         explorer.exe
// @architecture    amd64
// @compilerOptions -ldwmapi
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Native Smooth Taskbar

A small, experimental Windows 11 x64 taskbar companion. Windows continues to
render its own minimize, restore, open and close animations. This mod does not
change their duration, increase their frame rate, or fix a graphics-driver stall.

## What it changes

* Grouped buttons can use Explorer's existing Ctrl+click cycling action on an
  ordinary click, opening a window without first choosing a thumbnail.
* Optionally keep the active app open when its taskbar button is clicked again.
* Optional nonblocking minimize/restore requests for eligible desktop windows.
  These are experimental, OFF by default, and only affect calls made inside
  Explorer's native task switching routine. Some apps use other paths.
* Ctrl, Shift or Alt held down bypasses customization by default.
* Exact executable-name exclusions for the experimental request path.

Native open/close animations are preserved, not accelerated. There is no custom
renderer, screen capture, window cloaking, animation-completion wait, polling
thread, high-resolution timer request, or process-priority change. No app or DWM
injection is used. There is no additional per-frame rendering workload.

## Installation and recommended starting point

In Windhawk, choose Create a new mod, replace the template with this entire
file, then Compile Mod. Use current Windhawk on Windows 11 x64 with the standard
taskbar. ARM64, ExplorerPatcher and StartAllBack are not supported targets.
Microsoft symbols must be reachable for the first load. Missing required symbols
cause initialization to fail without applying the mod's hooks.

Start with the defaults. Enable Windows Settings > Accessibility > Visual
effects > Animation effects if you want native animations. The mod respects
that setting and does not rewrite system preferences.

For a grouped app, repeated clicks cycle its windows. For a single-window app,
the default toggle behavior remains native. Enable Keep active app open if you
prefer repeated clicks to leave it visible. No click debounce is introduced.

Test each experimental async setting separately. Restore requests are handed to
the app's own thread; the window might not be foreground immediately. A hung app
cannot process its request until it responds. If activation, rapid clicking or
maximized restoration behaves differently, turn that setting off. Windows still
decides animation timing and focus policy. There is no forced focus takeover.

The exclusions apply ONLY to the async path. Modal/owned windows, cloaked
windows, Explorer's own windows, unknown process identities and unsupported
requests use the original path. Failed async submissions also use the original
path. Once a request has been accepted, it is not submitted a second time.

## Compatibility and validation

Version 0.1.0 is a source release: cross-compilation and automated policy tests
are provided in the repository. It has NOT been run in a real Windows desktop
in the development environment. There is no measured FPS, latency or battery
claim. Test before relying on it. Disable this mod to remove its hooks; settings
changes reload it. No registry changes require undoing.

If your existing opening/closing flash also occurs without animation mods, this
mod does not diagnose it. Check that separately with native animations, your
display refresh rate and the graphics driver. Do not stack animation replacement
mods when evaluating native behavior.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- enabled: true
  $name: Enable taskbar customization
- groupClick: cycle
  $name: Click a grouped app
  $description: Use Explorer's native cycling action to access a window directly, or keep the thumbnail chooser.
  $options:
  - cycle: Cycle windows directly
  - native: Windows default
- activeClick: toggle
  $name: Click the active app again
  $description: Toggle keeps normal minimize/restore behavior. Keep open prevents taskbar-click minimization; the title-bar minimize button still works.
  $options:
  - toggle: Windows default toggle
  - keep: Keep active app open
- respectModifiers: true
  $name: Hold Ctrl, Shift or Alt for native behavior
  $description: Preserve Windows' modified-click actions and bypass this mod while a modifier is held.
- asyncMinimize: false
  $name: Experimental nonblocking minimize
  $description: Hand eligible taskbar ShowWindow minimize calls to the app's thread. May change rapid-click behavior; off is the compatibility default.
- asyncRestore: false
  $name: Experimental nonblocking restore
  $description: Hand eligible minimized-window restore calls to the app's thread. Focus may arrive later. Does not make an app render faster; test separately before relying on it.
- asyncExclusions: ""
  $name: Apps excluded from experimental async requests
  $description: Semicolon-separated exact executable names, for example game.exe;editor.exe. Case-insensitive, no paths or wildcards. Other taskbar settings still apply.
- diagnostics: false
  $name: Diagnostic logging
  $description: Log modified requests and native fallbacks when Windhawk logging is enabled. Leave off for normal use. Logs contain window handles, not window titles.
*/
// ==/WindhawkModSettings==

#include <string_view>

// Pure decisions are compiled unchanged by tests/policy_tests.cpp.
namespace NativeSmooth {
enum class Request { Other, Minimize, Restore };

constexpr bool ShouldCustomize(bool enabled, bool respectModifiers,
                               bool modifierDown) {
    return enabled && !(respectModifiers && modifierDown);
}

constexpr int ClickAction(int original, int groupType, bool cycle,
                          bool customize) {
    // Explorer's eCLICKACTION: 0 = ordinary click, 4 = native Ctrl+click cycle.
    // Group type 3 is a combined group; type 2 is a pinned launcher.
    return customize && cycle && groupType == 3 && original == 0 ? 4 : original;
}

constexpr bool ShouldQueue(Request request, bool minimizeEnabled,
                           bool restoreEnabled, bool eligible, bool iconic) {
    if (!eligible) return false;
    if (request == Request::Minimize) return minimizeEnabled && !iconic;
    if (request == Request::Restore) return restoreEnabled && iconic;
    return false;
}

constexpr wchar_t FoldAscii(wchar_t value) {
    return value >= L'A' && value <= L'Z' ? value + (L'a' - L'A') : value;
}

constexpr bool EqualName(std::wstring_view a, std::wstring_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (FoldAscii(a[i]) != FoldAscii(b[i])) return false;
    }
    return true;
}

constexpr bool IsSpace(wchar_t ch) {
    return ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n';
}

constexpr std::wstring_view Trim(std::wstring_view text) {
    while (!text.empty() && IsSpace(text.front())) text.remove_prefix(1);
    while (!text.empty() && IsSpace(text.back())) text.remove_suffix(1);
    return text;
}

// Shared parsing; the Windows adapter supplies Unicode-aware name comparison.
template <typename Equal>
bool Excluded(std::wstring_view names, std::wstring_view executable, Equal equal) {
    if (executable.empty()) return true;  // Unknown identity: native fallback.
    while (!names.empty()) {
        const auto end = names.find(L';');
        const auto name = Trim(names.substr(0, end));
        if (!name.empty() && equal(name, executable)) return true;
        if (end == std::wstring_view::npos) break;
        names.remove_prefix(end + 1);
    }
    return false;
}
}  // namespace NativeSmooth

#ifndef NATIVE_SMOOTH_POLICY_TEST

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#include <windhawk_api.h>

#include <atomic>
#include <cwchar>
#include <string>

namespace {
struct Settings {
    bool enabled = true;
    bool cycleGroups = true;
    bool keepActive = false;
    bool respectModifiers = true;
    bool asyncMinimize = false;
    bool asyncRestore = false;
    bool diagnostics = false;
    std::wstring exclusions;
} g_settings;

// Settings are immutable while hooks are active. Settings changes request a
// Windhawk reload, avoiding locks or concurrent string mutations on click paths.
std::atomic<bool> g_unloading{false};
thread_local unsigned int g_switchDepth = 0;
thread_local unsigned int g_apiDepth = 0;

class DepthScope {
public:
    explicit DepthScope(unsigned int& depth) : depth_(depth) { ++depth_; }
    ~DepthScope() { --depth_; }
    DepthScope(const DepthScope&) = delete;
    DepthScope& operator=(const DepthScope&) = delete;
private:
    unsigned int& depth_;
};

using SwitchToFn = HRESULT(WINAPI*)(void*, void*, BOOL);
using HandleClickFn = void(WINAPI*)(void*, void*, int, int, int, int);
using GroupTypeFn = int(WINAPI*)(void*);
SwitchToFn g_switchToOriginal = nullptr;
HandleClickFn g_handleClickOriginal = nullptr;
GroupTypeFn g_groupType = nullptr;
decltype(&ShowWindow) g_showWindowOriginal = nullptr;
decltype(&SwitchToThisWindow) g_switchWindowOriginal = nullptr;

bool Customizing() {
    if (g_unloading.load(std::memory_order_relaxed)) return false;
    const bool modifier = (GetKeyState(VK_SHIFT) < 0) ||
                          (GetKeyState(VK_CONTROL) < 0) ||
                          (GetKeyState(VK_MENU) < 0);
    return NativeSmooth::ShouldCustomize(g_settings.enabled,
                                        g_settings.respectModifiers, modifier);
}

void LogRequest(PCWSTR action, HWND hwnd) {
    if (g_settings.diagnostics) Wh_Log(L"%s: hwnd=%p", action, hwnd);
}

std::wstring StringSetting(PCWSTR name) {
    PCWSTR value = Wh_GetStringSetting(name);
    if (!value) return {};
    struct FreeSetting {
        PCWSTR value;
        ~FreeSetting() { Wh_FreeStringSetting(value); }
    } release{value};
    return value;
}

void LoadSettings() {
    g_settings.enabled = Wh_GetIntSetting(L"enabled") != 0;
    g_settings.cycleGroups = StringSetting(L"groupClick") == L"cycle";
    g_settings.keepActive = StringSetting(L"activeClick") == L"keep";
    g_settings.respectModifiers = Wh_GetIntSetting(L"respectModifiers") != 0;
    g_settings.asyncMinimize = Wh_GetIntSetting(L"asyncMinimize") != 0;
    g_settings.asyncRestore = Wh_GetIntSetting(L"asyncRestore") != 0;
    g_settings.diagnostics = Wh_GetIntSetting(L"diagnostics") != 0;
    g_settings.exclusions = StringSetting(L"asyncExclusions");
}

bool EligibleWindow(HWND hwnd, bool minimizing) {
    if (!IsWindow(hwnd) || !IsWindowVisible(hwnd) || !IsWindowEnabled(hwnd) ||
        GetAncestor(hwnd, GA_ROOT) != hwnd || GetWindow(hwnd, GW_OWNER)) {
        return false;
    }
    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if ((style & WS_CHILD) || (minimizing && !(style & WS_MINIMIZEBOX))) {
        return false;
    }
    const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (exStyle & (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE)) return false;
    const HWND popup = GetLastActivePopup(hwnd);
    if (popup && popup != hwnd && IsWindowVisible(popup)) return false;

    DWORD cloaked = 0;
    if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked,
                                     sizeof(cloaked))) || cloaked) return false;

    DWORD pid = 0;
    const DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
    if (!tid || !pid || tid == GetCurrentThreadId() ||
        pid == GetCurrentProcessId()) return false;

    // An inaccessible target is not treated as an unexcluded app. This also
    // avoids guessing at protected/packaged process identity.
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return false;
    wchar_t path[32768];
    DWORD chars = ARRAYSIZE(path);
    const BOOL gotName = QueryFullProcessImageNameW(process, 0, path, &chars);
    CloseHandle(process);
    if (!gotName || chars == 0 || chars >= ARRAYSIZE(path)) return false;
    const std::wstring_view fullPath(path, chars);
    const auto slash = fullPath.find_last_of(L"\\/");
    const auto name = fullPath.substr(slash == std::wstring_view::npos ? 0
                                                                       : slash + 1);
    const auto equal = [](std::wstring_view a, std::wstring_view b) {
        return CompareStringOrdinal(a.data(), static_cast<int>(a.size()),
                                    b.data(), static_cast<int>(b.size()), TRUE)
               == CSTR_EQUAL;
    };
    return !NativeSmooth::Excluded(g_settings.exclusions, name, equal);
}

bool TryQueue(HWND hwnd, NativeSmooth::Request request) {
    // Called only by API hooks nested inside CTaskBand::SwitchTo. No sleep,
    // retry queue, background thread, or wait for animation completion.
    const bool iconic = IsIconic(hwnd) != FALSE;
    if (!NativeSmooth::ShouldQueue(request, g_settings.asyncMinimize,
                                   g_settings.asyncRestore, true, iconic)) {
        return false;
    }
    if (!EligibleWindow(hwnd, request == NativeSmooth::Request::Minimize)) {
        LogRequest(L"Native compatibility fallback", hwnd);
        return false;
    }
    const int command = request == NativeSmooth::Request::Minimize
                            ? SW_MINIMIZE : SW_RESTORE;
    if (!ShowWindowAsync(hwnd, command)) {
        LogRequest(L"Async submission failed; using native call", hwnd);
        return false;
    }
    LogRequest(command == SW_MINIMIZE ? L"Queued minimize" : L"Queued restore",
               hwnd);
    return true;
}

BOOL WINAPI ShowWindowHook(HWND hwnd, int command) {
    const DWORD entryError = GetLastError();
    // Reentrancy guard prevents a fallback native API call from being modified
    // a second time by another hook lower in its call stack.
    DepthScope apiScope(g_apiDepth);
    if (g_apiDepth == 1 && g_switchDepth && Customizing()) {
        NativeSmooth::Request request = NativeSmooth::Request::Other;
        if (command == SW_MINIMIZE) request = NativeSmooth::Request::Minimize;
        if (command == SW_RESTORE) request = NativeSmooth::Request::Restore;
        const BOOL wasVisible = IsWindowVisible(hwnd);
        if (TryQueue(hwnd, request)) {
            SetLastError(entryError);
            // ShowWindow returns PREVIOUS visibility, not submission success.
            return wasVisible;
        }
    }
    SetLastError(entryError);
    return g_showWindowOriginal(hwnd, command);
}

void WINAPI SwitchWindowHook(HWND hwnd, BOOL altTab) {
    const DWORD entryError = GetLastError();
    DepthScope apiScope(g_apiDepth);
    if (g_apiDepth == 1 && g_switchDepth && Customizing() &&
        TryQueue(hwnd, NativeSmooth::Request::Restore)) {
        SetLastError(entryError);
        return;
    }
    SetLastError(entryError);
    g_switchWindowOriginal(hwnd, altTab);
}

HRESULT WINAPI SwitchToHook(void* self, void* item, BOOL noMinimize) {
    const DWORD entryError = GetLastError();
    if (!Customizing()) {
        SetLastError(entryError);
        return g_switchToOriginal(self, item, noMinimize);
    }
    DepthScope switchScope(g_switchDepth);
    SetLastError(entryError);
    return g_switchToOriginal(self, item,
                              g_settings.keepActive ? TRUE : noMinimize);
}

void WINAPI HandleClickHook(void* self, void* group, int index, int action,
                            int param4, int param5) {
    const DWORD entryError = GetLastError();
    if (group && Customizing() && g_settings.cycleGroups && action == 0) {
        action = NativeSmooth::ClickAction(action, g_groupType(group), true, true);
        if (action == 4 && g_settings.diagnostics) {
            Wh_Log(L"Grouped click routed to native window cycling");
        }
    }
    SetLastError(entryError);
    g_handleClickOriginal(self, group, index, action, param4, param5);
}

struct SymbolTarget {
    PCWSTR name;
    void* address = nullptr;
};

bool FindSymbols(HMODULE module, SymbolTarget* targets, size_t count) {
    WH_FIND_SYMBOL symbol{};
    HANDLE search = Wh_FindFirstSymbol(module, nullptr, &symbol);
    if (!search) {
        Wh_Log(L"Cannot load taskbar symbols. Check Windhawk's symbol download log.");
        return false;
    }
    size_t found = 0;
    do {
        for (size_t i = 0; i < count; ++i) {
            if (!targets[i].address && symbol.symbol && symbol.address &&
                wcscmp(symbol.symbol, targets[i].name) == 0) {
                targets[i].address = symbol.address;
                ++found;
            }
        }
        if (found == count) break;
    } while (Wh_FindNextSymbol(search, &symbol));
    Wh_FindCloseSymbol(search);
    if (found != count) {
        for (size_t i = 0; i < count; ++i) {
            if (!targets[i].address) Wh_Log(L"Required symbol missing: %s", targets[i].name);
        }
        return false;
    }
    return true;
}

template <typename Function>
bool InstallHook(Function target, Function hook, Function* original) {
    return target && Wh_SetFunctionHook(reinterpret_cast<void*>(target),
                                        reinterpret_cast<void*>(hook),
                                        reinterpret_cast<void**>(original));
}
}  // namespace

BOOL Wh_ModInit() {
    LoadSettings();
    if (!g_settings.enabled) return TRUE;
    using VersionFn = void(WINAPI*)(DWORD*, DWORD*, DWORD*);
    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    const auto version = ntdll ? reinterpret_cast<VersionFn>(
                                    GetProcAddress(ntdll, "RtlGetNtVersionNumbers"))
                              : nullptr;
    DWORD major = 0, minor = 0, build = 0;
    if (!version) return FALSE;
    version(&major, &minor, &build);
    build &= 0x0FFFFFFF;
    if (major != 10 || build < 22000) {
        Wh_Log(L"This version targets the standard Windows 11 x64 taskbar.");
        return FALSE;
    }

    // Use only the taskbar DLL already loaded by Explorer. Do not load a shell
    // DLL into an unsupported process or guess instruction offsets.
    HMODULE taskbar = GetModuleHandleW(L"taskbar.dll");
    if (!taskbar) {
        Wh_Log(L"Taskbar.dll is not loaded. Enable the mod after Explorer starts.");
        return FALSE;
    }
    SymbolTarget targets[] = {
        {L"public: virtual long __cdecl CTaskBand::SwitchTo(struct ITaskItem *,int)"},
        {L"protected: void __cdecl CTaskListWnd::_HandleClick(struct ITaskBtnGroup *,int,enum CTaskListWnd::eCLICKACTION,int,int)"},
        {L"public: virtual enum eTBGROUPTYPE __cdecl CTaskBtnGroup::GetGroupType(void)"},
    };
    // Validate every required symbol before registering any hook.
    const size_t count = g_settings.cycleGroups ? ARRAYSIZE(targets) : 1;
    if (!FindSymbols(taskbar, targets, count)) return FALSE;
    if (!InstallHook(reinterpret_cast<SwitchToFn>(targets[0].address),
                     SwitchToHook, &g_switchToOriginal)) return FALSE;
    if (g_settings.cycleGroups) {
        g_groupType = reinterpret_cast<GroupTypeFn>(targets[2].address);
        if (!InstallHook(reinterpret_cast<HandleClickFn>(targets[1].address),
                         HandleClickHook, &g_handleClickOriginal)) return FALSE;
    }
    if (g_settings.asyncMinimize || g_settings.asyncRestore) {
        if (!InstallHook(&ShowWindow, ShowWindowHook, &g_showWindowOriginal)) {
            return FALSE;
        }
    }
    if (g_settings.asyncRestore &&
        !InstallHook(&SwitchToThisWindow, SwitchWindowHook,
                     &g_switchWindowOriginal)) return FALSE;
    Wh_Log(L"Native Smooth Taskbar initialized on build %lu; async minimize=%d, restore=%d",
           build, g_settings.asyncMinimize, g_settings.asyncRestore);
    return TRUE;
}

void Wh_ModBeforeUninit() {
    g_unloading.store(true, std::memory_order_relaxed);
}

void Wh_ModUninit() {
    // Windhawk removes hooks and waits for active hook calls. There are no
    // worker threads, window subclasses, timers, or global preferences to undo.
}

BOOL Wh_ModSettingsChanged(BOOL* reload) {
    *reload = TRUE;
    return TRUE;
}

#endif  // NATIVE_SMOOTH_POLICY_TEST

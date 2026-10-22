// Compile the actual Windows hook implementation against deterministic API
// doubles. This tests routing/failure behavior, not real Windows rendering.
#include "../native-smooth-taskbar.wh.cpp"

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace Test {
int checks = 0;
const HWND window = reinterpret_cast<HWND>(0x100);
const HWND other = reinterpret_cast<HWND>(0x200);
struct State {
    DWORD lastError = 77;
    bool modifier = false;
    bool valid = true, visible = true, enabled = true, iconic = false;
    bool child = false, owned = false, popup = false, tool = false;
    bool noActivate = false, minimizeBox = true, cloaked = false;
    bool dwmFails = false, ownProcess = false, ownThread = false;
    bool processFails = false, nameFails = false, asyncSucceeds = true;
    bool shellLoaded = true, missingSymbols = false, symbolsUnavailable = false;
    bool nestedNativeApi = false, nativeSwitchCallsShow = false;
    int showCalls = 0, switchCalls = 0, asyncCalls = 0, closedHandles = 0;
    int bandCalls = 0, clickCalls = 0, lastCommand = -1, groupType = 3;
    int clickAction = -1, hookCalls = 0, failedHook = 0, searchCloses = 0;
    int nativeShowResult = 42;
    BOOL noMinimize = FALSE;
    std::wstring processName = L"C:\\Apps\\example.exe";
    std::map<std::wstring, int> ints{{L"enabled", 1}, {L"respectModifiers", 1}};
    std::map<std::wstring, std::wstring> strings{{L"groupClick", L"cycle"},
                                              {L"activeClick", L"toggle"}};
    size_t symbolIndex = 0;
} state;

void Check(bool condition, const char* description) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        std::exit(1);
    }
}

HRESULT WINAPI NativeBand(void*, void*, BOOL noMinimize) {
    ++state.bandCalls;
    state.noMinimize = noMinimize;
    if (state.nativeSwitchCallsShow) ShowWindowHook(window, SW_MINIMIZE);
    SetLastError(555);
    return 123;
}

void WINAPI NativeClick(void*, void*, int, int action, int, int) {
    ++state.clickCalls;
    state.clickAction = action;
    SetLastError(444);
}

int WINAPI NativeGroupType(void*) { return state.groupType; }

void Reset() {
    state = {};
    g_settings = {};
    g_unloading = false;
    g_switchDepth = 0;
    g_apiDepth = 0;
    g_switchToOriginal = NativeBand;
    g_handleClickOriginal = NativeClick;
    g_groupType = NativeGroupType;
    g_showWindowOriginal = ShowWindow;
    g_switchWindowOriginal = SwitchToThisWindow;
}

const wchar_t* symbols[] = {
    L"public: virtual long __cdecl CTaskBand::SwitchTo(struct ITaskItem *,int)",
    L"protected: void __cdecl CTaskListWnd::_HandleClick(struct ITaskBtnGroup *,int,enum CTaskListWnd::eCLICKACTION,int,int)",
    L"public: virtual enum eTBGROUPTYPE __cdecl CTaskBtnGroup::GetGroupType(void)",
};

void SetSymbol(WH_FIND_SYMBOL* symbol) {
    symbol->symbol = state.missingSymbols ? L"Unknown::Symbol()" : symbols[state.symbolIndex];
    void* addresses[] = {reinterpret_cast<void*>(NativeBand),
                         reinterpret_cast<void*>(NativeClick),
                         reinterpret_cast<void*>(NativeGroupType)};
    symbol->address = addresses[state.symbolIndex];
}
}  // namespace Test

DWORD GetLastError() { return Test::state.lastError; }
void SetLastError(DWORD value) { Test::state.lastError = value; }
short GetKeyState(int) { return Test::state.modifier ? -1 : 0; }
BOOL IsWindow(HWND) { return Test::state.valid; }
BOOL IsWindowVisible(HWND) { return Test::state.visible; }
BOOL IsWindowEnabled(HWND) { return Test::state.enabled; }
BOOL IsIconic(HWND) { return Test::state.iconic; }
HWND GetAncestor(HWND hwnd, int) { return Test::state.child ? Test::other : hwnd; }
HWND GetWindow(HWND, int) { return Test::state.owned ? Test::other : nullptr; }
LONG_PTR GetWindowLongPtrW(HWND, int kind) {
    const auto& s = Test::state;
    if (kind == GWL_STYLE) return (s.child ? WS_CHILD : 0) | (s.minimizeBox ? WS_MINIMIZEBOX : 0);
    return (s.tool ? WS_EX_TOOLWINDOW : 0) | (s.noActivate ? WS_EX_NOACTIVATE : 0);
}
HWND GetLastActivePopup(HWND hwnd) { return Test::state.popup ? Test::other : hwnd; }
DWORD GetWindowThreadProcessId(HWND, DWORD* pid) {
    *pid = Test::state.ownProcess ? 1 : 2;
    return Test::state.ownThread ? 10 : 20;
}
DWORD GetCurrentThreadId() { return 10; }
DWORD GetCurrentProcessId() { return 1; }
HANDLE OpenProcess(DWORD, BOOL, DWORD) { return Test::state.processFails ? nullptr : Test::other; }
BOOL QueryFullProcessImageNameW(HANDLE, DWORD, wchar_t* out, DWORD* count) {
    if (Test::state.nameFails) return FALSE;
    const auto& name = Test::state.processName;
    if (name.size() >= *count) return FALSE;
    std::wmemcpy(out, name.c_str(), name.size() + 1);
    *count = static_cast<DWORD>(name.size());
    return TRUE;
}
BOOL CloseHandle(HANDLE) { ++Test::state.closedHandles; return TRUE; }
int CompareStringOrdinal(const wchar_t* a, int alen, const wchar_t* b, int blen, BOOL) {
    return NativeSmooth::EqualName({a, static_cast<size_t>(alen)},
                                   {b, static_cast<size_t>(blen)}) ? CSTR_EQUAL : 1;
}
HRESULT DwmGetWindowAttribute(HWND, DWORD, void* out, DWORD) {
    *static_cast<DWORD*>(out) = Test::state.cloaked;
    return Test::state.dwmFails ? -1 : 0;
}
BOOL ShowWindow(HWND hwnd, int command) {
    ++Test::state.showCalls;
    Test::state.lastCommand = command;
    if (Test::state.nestedNativeApi) SwitchWindowHook(hwnd, TRUE);
    SetLastError(666);
    return Test::state.nativeShowResult;
}
BOOL ShowWindowAsync(HWND, int command) {
    ++Test::state.asyncCalls;
    Test::state.lastCommand = command;
    SetLastError(999);
    return Test::state.asyncSucceeds;
}
void SwitchToThisWindow(HWND, BOOL) { ++Test::state.switchCalls; SetLastError(888); }
HMODULE GetModuleHandleW(PCWSTR name) {
    if (std::wcscmp(name, L"taskbar.dll") == 0 && !Test::state.shellLoaded) return nullptr;
    return Test::other;
}
void WINAPI Version(DWORD* major, DWORD* minor, DWORD* build) {
    *major = 10; *minor = 0; *build = 26100;
}
FARPROC GetProcAddress(HMODULE, const char*) { return reinterpret_cast<FARPROC>(Version); }
void Wh_Log(PCWSTR, ...) {}
int Wh_GetIntSetting(PCWSTR name) { return Test::state.ints[name]; }
PCWSTR Wh_GetStringSetting(PCWSTR name) { return Test::state.strings[name].c_str(); }
void Wh_FreeStringSetting(PCWSTR) {}
HANDLE Wh_FindFirstSymbol(HMODULE, const void*, WH_FIND_SYMBOL* symbol) {
    if (Test::state.symbolsUnavailable) return nullptr;
    Test::state.symbolIndex = 0;
    Test::SetSymbol(symbol);
    return Test::other;
}
BOOL Wh_FindNextSymbol(HANDLE, WH_FIND_SYMBOL* symbol) {
    if (++Test::state.symbolIndex >= ARRAYSIZE(Test::symbols)) return FALSE;
    Test::SetSymbol(symbol);
    return TRUE;
}
void Wh_FindCloseSymbol(HANDLE) { ++Test::state.searchCloses; }
BOOL Wh_SetFunctionHook(void* target, void*, void** original) {
    if (++Test::state.hookCalls == Test::state.failedHook) return FALSE;
    *original = target;
    return TRUE;
}

int main() {
    using namespace Test;
    Reset();
    g_settings.asyncMinimize = true;
    Check(ShowWindowHook(window, SW_MINIMIZE) == 42 && state.asyncCalls == 0,
          "API calls outside task switching remain native");
    Check(GetLastError() == 666, "native ShowWindow last-error is preserved");

    Reset(); g_switchDepth = 1; g_settings.asyncMinimize = true;
    Check(ShowWindowHook(window, SW_MINIMIZE) == TRUE, "queued ShowWindow returns previous visibility");
    Check(state.asyncCalls == 1 && state.showCalls == 0, "accepted request submitted once");
    Check(GetLastError() == 77, "queued request restores caller last-error");
    Check(state.closedHandles == 1, "process handle closed after successful query");
    Check(g_apiDepth == 0, "API scope unwinds after early return");

    Reset(); g_switchDepth = 1; g_settings.asyncMinimize = true; state.asyncSucceeds = false;
    Check(ShowWindowHook(window, SW_MINIMIZE) == 42, "failed submission uses native return value");
    Check(state.asyncCalls == 1 && state.showCalls == 1, "failed submission falls back exactly once");
    Check(GetLastError() == 666, "fallback preserves native last-error");

    const std::vector<bool State::*> blockers = {
        &State::child, &State::owned, &State::popup, &State::tool, &State::noActivate,
        &State::cloaked, &State::dwmFails, &State::ownProcess, &State::ownThread,
        &State::processFails, &State::nameFails,
    };
    for (auto blocker : blockers) {
        Reset(); g_switchDepth = 1; g_settings.asyncMinimize = true; state.*blocker = true;
        ShowWindowHook(window, SW_MINIMIZE);
        Check(state.showCalls == 1 && state.asyncCalls == 0, "unsafe target remains native");
    }
    for (auto required : {&State::valid, &State::visible, &State::enabled, &State::minimizeBox}) {
        Reset(); g_switchDepth = 1; g_settings.asyncMinimize = true; state.*required = false;
        ShowWindowHook(window, SW_MINIMIZE);
        Check(state.showCalls == 1 && state.asyncCalls == 0, "missing window capability stays native");
    }
    Reset(); g_switchDepth = 1; g_settings.asyncMinimize = true;
    g_settings.exclusions = L"other.exe; EXAMPLE.exe";
    ShowWindowHook(window, SW_MINIMIZE);
    Check(state.asyncCalls == 0 && state.showCalls == 1, "excluded process uses native path");
    Check(state.closedHandles == 1, "excluded process handle is closed");

    Reset(); g_switchDepth = 1; g_settings.asyncRestore = true; state.iconic = true;
    SwitchWindowHook(window, TRUE);
    Check(state.asyncCalls == 1 && state.switchCalls == 0 && state.lastCommand == SW_RESTORE,
          "iconic SwitchToThisWindow queues native restore");
    Check(GetLastError() == 77, "switch enqueue preserves incoming last-error");
    Reset(); g_switchDepth = 1; g_settings.asyncRestore = true;
    ShowWindowHook(window, SW_RESTORE);
    Check(state.asyncCalls == 0 && state.showCalls == 1,
          "non-iconic restore preserves native maximize semantics");
    Reset(); g_switchDepth = 1; g_settings.asyncRestore = true; state.iconic = true;
    state.asyncSucceeds = false;
    SwitchWindowHook(window, FALSE);
    Check(state.asyncCalls == 1 && state.switchCalls == 1,
          "switch enqueue failure calls native switch exactly once");
    Check(GetLastError() == 888, "native switch error preserved after fallback");

    Reset(); g_switchDepth = 1; g_settings.asyncRestore = true; state.iconic = true;
    state.nestedNativeApi = true;
    ShowWindowHook(window, SW_SHOW);
    Check(state.showCalls == 1 && state.switchCalls == 1 && state.asyncCalls == 0,
          "nested native fallback is not intercepted again");
    Check(g_apiDepth == 0, "nested API depth restored");

    Reset(); g_settings.keepActive = true;
    Check(SwitchToHook(nullptr, nullptr, FALSE) == 123 && state.noMinimize,
          "keep-open uses native noMinimize and preserves HRESULT");
    Check(g_switchDepth == 0 && GetLastError() == 555,
          "task-switch scope unwinds without losing native last-error");
    Reset(); g_settings.asyncMinimize = true; state.nativeSwitchCallsShow = true;
    SwitchToHook(nullptr, nullptr, FALSE);
    Check(state.asyncCalls == 1 && g_switchDepth == 0,
          "API interception occurs inside actual task-switch hook scope");
    Reset(); g_settings.keepActive = true; state.modifier = true;
    SwitchToHook(nullptr, nullptr, FALSE);
    Check(!state.noMinimize, "modifier bypass preserves native toggle");

    Reset(); HandleClickHook(nullptr, other, 0, 0, 0, 0);
    Check(state.clickAction == 4 && state.clickCalls == 1, "combined click routed once");
    Check(GetLastError() == 444, "click native last-error preserved");
    Reset(); state.groupType = 2; HandleClickHook(nullptr, other, 0, 0, 0, 0);
    Check(state.clickAction == 0, "pinned launchers are never cycled");
    Reset(); state.modifier = true; HandleClickHook(nullptr, other, 0, 0, 0, 0);
    Check(state.clickAction == 0, "modifier bypass preserves grouped click");
    Reset(); HandleClickHook(nullptr, nullptr, 0, 0, 0, 0);
    Check(state.clickAction == 0, "null group forwarded without lookup");

    Reset(); Wh_ModBeforeUninit();
    g_settings.keepActive = true; SwitchToHook(nullptr, nullptr, FALSE);
    Check(!state.noMinimize, "unloading stops customization");
    BOOL reload = FALSE;
    Check(Wh_ModSettingsChanged(&reload) == TRUE && reload,
          "settings callback returns BOOL success and requests reload");

    Reset(); state.shellLoaded = false;
    Check(!Wh_ModInit() && state.hookCalls == 0, "no taskbar DLL: no hooks");
    Reset(); state.missingSymbols = true;
    Check(!Wh_ModInit() && state.hookCalls == 0 && state.searchCloses == 1,
          "missing symbols: no hooks and symbol search closed");
    Reset(); state.symbolsUnavailable = true;
    Check(!Wh_ModInit() && state.hookCalls == 0, "symbol download failure: no hooks");
    Reset();
    Check(Wh_ModInit() && state.hookCalls == 2, "defaults install only two shell hooks");
    Reset(); state.ints[L"asyncMinimize"] = 1; state.ints[L"asyncRestore"] = 1;
    Check(Wh_ModInit() && state.hookCalls == 4, "API hooks installed only when enabled");
    Reset(); state.ints[L"enabled"] = 0;
    Check(Wh_ModInit() && state.hookCalls == 0, "disabled mod installs no hooks");
    Reset(); state.failedHook = 1;
    Check(!Wh_ModInit(), "hook registration failure propagates to Windhawk");
    std::cout << checks << " adapter checks passed\n";
}

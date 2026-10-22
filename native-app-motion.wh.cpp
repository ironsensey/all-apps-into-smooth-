// ==WindhawkMod==
// @id              native-app-motion
// @name            Native-style App Motion
// @description     Native-style GPU app transitions with a 60 Hz preset and synchronized window handoffs.
// @version         0.3.1
// @author          Native App Motion contributors
// @license         MIT
// @include         *
// @exclude         dwm.exe
// @exclude         csrss.exe
// @exclude         winlogon.exe
// @exclude         services.exe
// @exclude         lsass.exe
// @exclude         smss.exe
// @exclude         TextInputHost.exe
// @exclude         ShellExperienceHost.exe
// @exclude         StartMenuExperienceHost.exe
// @exclude         SearchHost.exe
// @architecture    amd64
// @compilerOptions -ldwmapi -lgdi32 -lole32 -ladvapi32 -luuid
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Native-style App Motion

This is the APP OPEN/CLOSE/RESTORE mod. The older native-smooth-taskbar file is a
separate taskbar experiment; it is not needed for this mod.

Version 0.3.1 is experimental and has not been run on a real Windows desktop in
the development environment. Cross-compilation is not a visual or compatibility
test. No zero-stutter, zero-bug, FPS or battery-life guarantee is made.

## Appearance and rendering

Recreates the familiar Windows scale-and-fade style. It is a custom approximation,
not Windows' private transition code or an exact copy of every Windows build's
easing, shadows and geometry.

Opening uses a live DWM thumbnail while the real window is temporarily cloaked.
The app continues to paint. The renderer updates thumbnail position/opacity on
composition boundaries only while openings are active. A one-shot watchdog in
the source process restores the real window if the renderer fails or stops.
Version 0.3.1 presents the final thumbnail and then the real source before
removing the overlay or restoring native transitions. Redundant show/no-op
layout calls preserve the animation. Repeated startup hide/show cycles do not
replay the custom launch effect on the same window. If Explorer cannot finish
the handoff, it asks the existing source recovery worker to finish locally.

Closing captures the unobscured window once and uploads it once. DirectComposition
then runs the fade/scale curves on the GPU. The renderer lives in Explorer, so a
last-window close can continue animating after the app exits. A short, bounded
preparation handoff precedes native destruction; the app does not wait for the
full closing animation. Failed/late preparation uses the original close.

Version 0.3 adds supported taskbar restoration of minimized windows. Explorer's
native SwitchTo routine is scoped through one optional symbol hook; intercepted
restore APIs forward a small request to the confirmed source UI dispatcher.
This path adds no capture or animation-completion wait to the taskbar thread.
Restore defaults are a 100 ms ease-out and 50% first-frame visibility. A deadline
watchdog handles unclaimed requests; an atomic claim prevents duplicate restores
that could otherwise unmaximize a window. New input can cancel stale requests.

Already-visible app switching and app startup/loading time remain native. Those
are different from restoring a minimized window; animation pace does not make
a busy app render sooner. Actual click-to-frame latency has not been measured.

There is no new taskbar click policy, per-frame screenshot capture, CPU pixel
animation loop, global timer-resolution request, or process-priority boost.
Memory/concurrency limits and battery settings are configurable. Animation costs
are bounded, not claimed to be lower than native Windows on every device.

## Install

Use current Windhawk on Windows 11 x64. Create a new mod, replace the template
with this WHOLE file and compile it. Enable it after Explorer is running. Restart
test apps so Windhawk can inject the mod. The Explorer renderer and app hooks
must both be loaded. Disable other window-animation replacement mods first.

Use Motion preset: Balanced 60 Hz (recommended). It applies 133 ms opening,
100 ms close/restore, 50% restore starting visibility, 97% scale, ease-out,
2 simultaneous effects, a 2 MP capture limit, a 34 ms close preparation deadline,
15 seconds GPU idle release and native effects on battery. This preserves full
1360x768 capture resolution; it is a starting preset, not a benchmarked optimum.
These are curve/deadline values, not measured total input-to-display times.
Saved individual tuning fields are inactive until Custom settings is selected.
Feature toggles, exclusions and logging remain independent. After upgrading from
0.3.0, restart Windows once so Explorer and all apps load the same version.
Update the existing mod; do not enable a second copy.

Test Notepad, a browser and the apps you actually
use. Keep Windows Animation effects enabled. Protected apps, different integrity
levels, custom taskbar/shell replacements, direct process termination and APIs
that bypass the hooks can fall back or remain native. Elevated apps can use their
native path if the renderer cannot access their request objects/window.

Supported hooks: first ShowWindow/SetWindowPos show, DestroyWindow close,
supported taskbar/own-thread restores, and optionally an explicit hide.
Minimize/maximize and unsupported restores remain native. Apps that
show a blank first frame may still do so: the mod cannot make app content ready
earlier. A live thumbnail must work for opening to render correctly; test this
on your Windows build. Closing skips overlapped/partly offscreen windows to avoid
animating pixels belonging to another app. Large snapshots are downscaled to the
configured pixel budget.

Rapid close/minimize while opening cancels the opening and restores the real
window before the native operation. It does not enqueue a stale opening after
the newer action. Disable/reload restores windows cloaked by this mod.

Use diagnostics to distinguish a custom animation from a native fallback. Enable
Diagnostic logging and Windhawk logging, restart the test app, minimize it, then
click its taskbar button. Confirm TASKBAR_HOOK_READY, BROKER_READY protocol=0.3.1,
CLIENT_READY, TASKBAR_RESTORE_QUEUED, RESTORE_RECEIVED and RESTORE_ANIMATION_STARTED.
If the last line is absent, read the TASKBAR_NATIVE/RENDERER_NATIVE explanation.
TASKBAR_DISPATCH duration is API time, not click-to-frame latency. Optional taskbar
symbols may need Microsoft's symbol server on first use. Missing symbols leave
this restore path native; no offsets are guessed.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- motionPreset: balanced60
  $name: Motion preset
  $description: Balanced 60 Hz applies the timing/resource values together, including after updating an older version. Select Custom to use the individual tuning fields below. Animation toggles, exclusions and logging remain independent.
  $options:
  - balanced60: Balanced 60 Hz (recommended)
  - custom: Custom settings
- animateOpen: true
  $name: Animate app opening
  $description: Recreate the Windows-style fade/scale using a live GPU-composited window thumbnail. Experimental; test on your Windows build.
- animateClose: true
  $name: Animate app closing
  $description: Recreate the Windows-style fade/scale using a single captured image and compositor-driven GPU animation.
- animateRestore: true
  $name: Animate restoration from the taskbar
  $description: Restore supported minimized apps through their UI thread and apply a fast-start GPU fade/scale. Requires the new version in Explorer and the target app. Native fallback is used when routing is unavailable.
- restoreDurationMs: 100
  $name: Custom - Taskbar restore duration (ms)
  $description: Clamped to 60–250 ms. Changes transition pace, not how quickly a busy app can render.
- restoreStartOpacityPercent: 50
  $name: Custom - Restore first-frame visibility (%)
  $description: Clamped to 0–100. A visible initial frame gives earlier visual feedback instead of starting completely transparent.
- restoreDispatchTimeoutMs: 50
  $name: Custom - Restore request dispatch deadline (ms)
  $description: Clamped to 20–120. If the app has not claimed a queued restore by this deadline, the renderer submits a native restore. A claimed request is never submitted twice.
- openDurationMs: 133
  $name: Custom - Opening duration (ms)
  $description: Clamped to 80–400 ms. Longer animations are not inherently smoother.
- closeDurationMs: 100
  $name: Custom - Closing duration (ms)
  $description: Clamped to 60–350 ms. The source app does not wait for this whole duration.
- scalePercent: 97
  $name: Custom - Smallest window scale (%)
  $description: 97 gives a subtle Windows-style scale effect. 100 gives fade only. Clamped to 90–100.
- easing: out
  $name: Custom - Opening and closing motion curve
  $options:
  - smooth: Smooth start and finish
  - out: Fast start, gentle finish
  - linear: Linear
- maxConcurrent: 2
  $name: Custom - Maximum simultaneous animations
  $description: Clamped to 1–4. Additional requests use native behavior.
- snapshotMegapixels: 2
  $name: Custom - Maximum closing snapshot size (megapixels)
  $description: Clamped to 1–8. Larger windows are downscaled; this limits CPU upload and texture size. A short fade may look softer on very large windows.
- handoffTimeoutMs: 34
  $name: Custom - Maximum closing renderer handoff wait (ms)
  $description: Clamped to 8–80. A slow renderer falls back to the native close. This excludes capture/native API execution time.
- gpuIdleSeconds: 15
  $name: Custom - Release closing GPU resources after idle (seconds)
  $description: Clamped to 1–120. Releases the closing renderer's device when unused. A later cold start may use native closing if it misses the handoff deadline.
- animateTrayHides: false
  $name: Also animate explicit hides
  $description: Applies to ShowWindow(SW_HIDE) and SetWindowPos hide requests. Off avoids treating ordinary visibility changes as closes.
- disableOnBattery: true
  $name: Custom - Use native animations on battery
  $description: Skip this mod's effects while Windows reports battery power. No power-plan settings are changed.
- respectAnimationEffects: true
  $name: Respect Windows animation preferences
  $description: Use native behavior when Windows client-area animations are disabled.
- excludedProcesses: ""
  $name: Excluded apps
  $description: Semicolon-separated exact executable basenames, for example game.exe;editor.exe. Explorer must remain enabled for the renderer to run.
- diagnostics: false
  $name: Diagnostic logging
  $description: Log actual custom animations and fallbacks when Windhawk logging is enabled. Window titles and image pixels are not logged or saved.
*/
// ==/WindhawkModSettings==

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

namespace AppMotion {
enum class Curve { Smooth, Out, Linear };
enum : int { Pending, Ready, Started, Complete, Cancelled };
constexpr bool RevealStateMatches(bool restoring,bool iconic,bool visible) {
    return restoring ? iconic : (!iconic && !visible);
}
constexpr bool CanRouteRestore(bool enabled,bool minimized,bool clientReady,
                               bool brokerReady,bool pending,bool hung) {
    return enabled && minimized && clientReady && brokerReady && !pending && !hung;
}
constexpr bool DisplayContentAllowed(bool snapshot,bool queried,std::uint32_t affinity,
                                     bool tracked,std::uint32_t trackedAffinity) {
    if (queried) return affinity==0;
    if (tracked) return trackedAffinity==0;
    // Do not treat an unsupported getter as proof that screen capture is
    // permitted. A live DWM thumbnail still observes compositor protection.
    return !snapshot;
}
template <typename CompareExchange>
bool ClaimRestore(CompareExchange compareExchange) {
    return compareExchange(Started,Pending)==Pending;
}
template <typename CompareExchange>
bool ClaimRestoreFallback(CompareExchange compareExchange) {
    return compareExchange(Cancelled,Pending)==Pending;
}
template <typename CompareExchange>
bool PublishReady(CompareExchange compareExchange) {
    return compareExchange(Ready,Pending)==Pending;
}
template <typename CompareExchange>
bool AcceptOrCancel(CompareExchange compareExchange) {
    // A renderer that wins the deadline race may proceed. If the caller wins,
    // Pending becomes Cancelled and the renderer cannot later publish Ready.
    return compareExchange(Cancelled,Pending)==Ready;
}
template <typename Platform>
bool RestoreOpening(Platform& platform) {
    if (!platform.Claim()) return false;
    if (!platform.OwnsGeneration()) return false;
    // On normal completion, present the full-size, opaque thumbnail first.
    // Keep it in place and transitions suppressed until the real source has
    // also been presented. Recovery/cancellation adapters can skip the waits.
    platform.PresentFinalFrame();
    const bool visible=platform.Uncloak();
    if (visible) platform.PresentSource();
    const bool preferences=platform.RestoreTransitions();
    if (!visible || !preferences) { platform.ReleaseClaim(); return false; }
    platform.RemoveToken();
    return true;
}
struct Rect { int left,top,right,bottom; };
constexpr bool ChangesGeometry(Rect before,int x,int y,int width,int height,bool move,bool size) {
    return (move && (before.left!=x || before.top!=y)) ||
           (size && (static_cast<long long>(before.right)-before.left!=width ||
                     static_cast<long long>(before.bottom)-before.top!=height));
}
struct Tuning {
    int openMs,closeMs,restoreMs,restoreOpacity,restoreDispatchMs;
    int scale,concurrent,megapixels,handoffMs,idleSeconds;
    Curve curve;
    bool battery;
};
// 8 opening intervals / 6 closing and restore intervals at 60 Hz, before
// presentation handoff. A 2 MP cap preserves a 1360x768 capture at full size.
constexpr Tuning Balanced60{133,100,100,50,50,97,2,2,34,15,Curve::Out,true};
constexpr int Clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
inline float Ease(float t, Curve curve) {
    t = std::clamp(t, 0.0f, 1.0f);
    if (curve == Curve::Linear) return t;
    if (curve == Curve::Out) return 1.0f - (1.0f-t)*(1.0f-t)*(1.0f-t);
    return t*t*(3.0f-2.0f*t);
}
struct Cubic { float a, b, c, d; };
inline Cubic Coefficients(float from, float to, float seconds, Curve curve) {
    const float delta = to-from;
    if (curve == Curve::Linear) return {from, delta/seconds, 0, 0};
    if (curve == Curve::Out) return {from, 3*delta/seconds,
                                   -3*delta/(seconds*seconds),
                                   delta/(seconds*seconds*seconds)};
    return {from, 0, 3*delta/(seconds*seconds),
            -2*delta/(seconds*seconds*seconds)};
}
struct ImageSize { int width, height; };
inline ImageSize FitImage(int width, int height, std::uint64_t budget) {
    if (width <= 0 || height <= 0 || budget == 0) return {0,0};
    const double ratio = std::min(1.0, std::sqrt(static_cast<double>(budget) /
                                (static_cast<double>(width)*height)));
    ImageSize size{std::max(1, static_cast<int>(width*ratio)),
                   std::max(1, static_cast<int>(height*ratio))};
    if (static_cast<std::uint64_t>(size.width)*size.height>budget) {
        if (size.width>=size.height) size.width=static_cast<int>(budget/size.height);
        else size.height=static_cast<int>(budget/size.width);
    }
    return size;
}
constexpr bool ValidImage(int width, int height, std::uint64_t bytes) {
    return width > 0 && height > 0 && width <= 16384 && height <= 16384 &&
           static_cast<std::uint64_t>(width)*height <= 8'000'000 &&
           bytes == static_cast<std::uint64_t>(width)*height*4;
}
}  // namespace AppMotion

#ifndef APP_MOTION_CORE_TEST
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <d2d1_1.h>
#include <dcomp.h>
#include <dwmapi.h>
#include <wrl/client.h>
#include <windhawk_api.h>

#include <atomic>
#include <array>
#include <cstring>
#include <cwchar>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace {
using Microsoft::WRL::ComPtr;
constexpr wchar_t kBrokerClass[] = L"NativeAppMotion.Broker.031.8F23A10C";
constexpr wchar_t kOverlayClass[] = L"NativeAppMotion.Overlay.031.8F23A10C";
constexpr wchar_t kOpenToken[] = L"NativeAppMotion.OpenToken.031.8F23A10C";
constexpr wchar_t kTransitionPreference[] = L"NativeAppMotion.Transitions.031.8F23A10C";
constexpr wchar_t kClientReady[] = L"NativeAppMotion.ClientReady.031.8F23A10C";
constexpr wchar_t kRestorePending[] = L"NativeAppMotion.RestorePending.031.8F23A10C";
constexpr wchar_t kAffinityPreference[] = L"NativeAppMotion.Affinity.031.8F23A10C";
constexpr wchar_t kShownOnce[] = L"NativeAppMotion.ShownOnce.031.8F23A10C";
constexpr std::uintptr_t kClientVersion=0x4E414D04;
constexpr UINT kRequest = WM_APP+0x471;
constexpr UINT kStartClose = WM_APP+0x472;
constexpr UINT kWake = WM_APP+0x473;
constexpr DWORD kMagic = 0x324D414E;
using AppMotion::Pending; using AppMotion::Ready; using AppMotion::Started;
using AppMotion::Complete; using AppMotion::Cancelled;
enum : DWORD { Opening=1, Closing=2, Restoring=3 };
bool LiveOperation(DWORD operation) { return operation==Opening || operation==Restoring; }

struct Packet {
    DWORD magic, version, headerBytes, operation, pid;
    DWORD durationMs, scalePercent, curve, startOpacityPercent;
    std::uint64_t token, source, pixelBytes, expires;
    RECT rect;
    int pixelWidth, pixelHeight;
    BOOL originalTransitions;
    volatile LONG state, restoreClaim;
};
static_assert(sizeof(void*) == 8, "This source targets Windows x64.");

struct Settings : AppMotion::Tuning {
    Settings() : Tuning(AppMotion::Balanced60) {}
    bool open=true, close=true, restore=true, hides=false, respectEffects=true;
    bool diagnostics=false;
    std::wstring excluded;
} g;
std::atomic<bool> g_unloading{false};
thread_local unsigned g_hookDepth=0;
thread_local unsigned g_taskbarDepth=0;
thread_local unsigned g_restoreRouteCount=0;
thread_local HWND g_taskbarRoutedWindow=nullptr;
thread_local DWORD g_taskbarInputTime=0;
thread_local bool g_taskbarInputSeen=false;
thread_local bool g_clientDispatcherSeen=false;
UINT g_restoreMessage=0,g_finishRevealMessage=0;
std::atomic<HANDLE> g_timerQueue{nullptr};
HANDLE g_stop=nullptr, g_brokerThread=nullptr;
DWORD g_brokerThreadId=0;
HINSTANCE g_module=nullptr;

decltype(&ShowWindow) g_show=nullptr;
decltype(&SetWindowPos) g_position=nullptr;
decltype(&DestroyWindow) g_destroy=nullptr;
decltype(&DwmSetWindowAttribute) g_setAttribute=nullptr;
decltype(&ShowWindowAsync) g_showAsync=nullptr;
decltype(&SwitchToThisWindow) g_switchWindow=nullptr;
decltype(&DispatchMessageW) g_dispatchW=nullptr;
decltype(&DispatchMessageA) g_dispatchA=nullptr;
decltype(&CreateWindowExW) g_createW=nullptr;
decltype(&CreateWindowExA) g_createA=nullptr;
decltype(&SetWindowDisplayAffinity) g_setAffinity=nullptr;
void MarkRestoreClient(HWND hwnd);
std::mutex g_affinityMutex;
std::atomic<bool> g_affinityReliable{true};
using TaskbarSwitchFn=HRESULT(WINAPI*)(void*,void*,BOOL);
TaskbarSwitchFn g_taskbarSwitch=nullptr;

HRESULT SetAttribute(HWND hwnd,DWORD attribute,LPCVOID value,DWORD size) {
    return g_setAttribute ? g_setAttribute(hwnd,attribute,value,size) :
                            DwmSetWindowAttribute(hwnd,attribute,value,size);
}
BOOL TransitionPreference(HWND hwnd,BOOL fallback) {
    const auto recorded=reinterpret_cast<std::uintptr_t>(GetPropW(hwnd,kTransitionPreference));
    return recorded==2 ? TRUE : recorded==1 ? FALSE : fallback;
}
HRESULT WINAPI AttributeHook(HWND hwnd,DWORD attribute,LPCVOID value,DWORD size) {
    const HRESULT result=g_setAttribute(hwnd,attribute,value,size);
    const DWORD error=GetLastError();
    if (!g_unloading && SUCCEEDED(result) && attribute==DWMWA_TRANSITIONS_FORCEDISABLED &&
        value && size==sizeof(BOOL)) {
        DWORD pid=0; GetWindowThreadProcessId(hwnd,&pid);
        if (pid==GetCurrentProcessId()) {
            BOOL preference=FALSE; memcpy(&preference,value,sizeof(preference));
            SetPropW(hwnd,kTransitionPreference,reinterpret_cast<HANDLE>(preference ? 2 : 1));
            // Defer the app's requested preference until our reveal finishes.
            // Re-enabling native transitions mid-reveal can produce a second
            // fade/scale when the real source is uncloaked.
            if (!preference && GetPropW(hwnd,kOpenToken)) {
                const BOOL disabled=TRUE;
                g_setAttribute(hwnd,DWMWA_TRANSITIONS_FORCEDISABLED,&disabled,sizeof(disabled));
            }
        }
    }
    SetLastError(error); return result;
}

BOOL WINAPI AffinityHook(HWND hwnd,DWORD affinity) {
    const BOOL result=g_setAffinity(hwnd,affinity);
    const DWORD error=GetLastError();
    if (result && !g_unloading) {
        std::lock_guard lock(g_affinityMutex);
        if (!SetPropW(hwnd,kAffinityPreference,
            reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(affinity)+1))) g_affinityReliable=false;
    }
    SetLastError(error); return result;
}
void MarkCreatedWindow(HWND hwnd,DWORD style,DWORD ex) {
    if (!hwnd || g_unloading || (style&WS_CHILD) || !(style&(WS_CAPTION|WS_THICKFRAME)) ||
        (ex&WS_EX_TOOLWINDOW)) return;
    // App setters invoked inside WM_CREATE have already populated the value.
    // Only new windows observed by these creation hooks get a known default.
    {
        std::lock_guard lock(g_affinityMutex);
        if (!GetPropW(hwnd,kAffinityPreference) &&
            !SetPropW(hwnd,kAffinityPreference,reinterpret_cast<HANDLE>(1))) g_affinityReliable=false;
    }
    if (g_clientDispatcherSeen) MarkRestoreClient(hwnd);
}
HWND WINAPI CreateWHook(DWORD ex,LPCWSTR cls,LPCWSTR title,DWORD style,int x,int y,int width,int height,
                        HWND parent,HMENU menu,HINSTANCE instance,LPVOID parameter) {
    HWND hwnd=g_createW(ex,cls,title,style,x,y,width,height,parent,menu,instance,parameter);
    const DWORD error=GetLastError(); MarkCreatedWindow(hwnd,style,ex); SetLastError(error); return hwnd;
}
HWND WINAPI CreateAHook(DWORD ex,LPCSTR cls,LPCSTR title,DWORD style,int x,int y,int width,int height,
                        HWND parent,HMENU menu,HINSTANCE instance,LPVOID parameter) {
    HWND hwnd=g_createA(ex,cls,title,style,x,y,width,height,parent,menu,instance,parameter);
    const DWORD error=GetLastError(); MarkCreatedWindow(hwnd,style,ex); SetLastError(error); return hwnd;
}

class ScopeDepth {
public:
    ScopeDepth() { ++g_hookDepth; }
    ~ScopeDepth() { --g_hookDepth; }
};
class DpiScope {
    DPI_AWARENESS_CONTEXT old_;
public:
    DpiScope() : old_(SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {}
    ~DpiScope() { if (old_) SetThreadDpiAwarenessContext(old_); }
};
struct Handle {
    HANDLE value=nullptr;
    Handle()=default;
    explicit Handle(HANDLE h) : value(h) {}
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    Handle(const Handle&)=delete;
    Handle& operator=(const Handle&)=delete;
};
void Log(PCWSTR message, HWND hwnd) {
    if (g.diagnostics) Wh_Log(L"%s: hwnd=%p", message, hwnd);
}
double PreciseMs() {
    static const double frequency=[] { LARGE_INTEGER f{}; QueryPerformanceFrequency(&f); return static_cast<double>(f.QuadPart); }();
    LARGE_INTEGER now{}; QueryPerformanceCounter(&now);
    return now.QuadPart*1000.0/frequency;
}
std::wstring GetString(PCWSTR name) {
    PCWSTR raw=Wh_GetStringSetting(name);
    if (!raw) return {};
    std::wstring value(raw);
    Wh_FreeStringSetting(raw);
    return value;
}
std::wstring ObjectName(DWORD pid, std::uint64_t token, PCWSTR suffix) {
    wchar_t name[160];
    swprintf(name, ARRAYSIZE(name), L"Local\\NativeAppMotion.031.%lu.%016llX.%s",
             pid, static_cast<unsigned long long>(token), suffix);
    return name;
}
bool SameSource(const Packet& p) {
    DWORD pid=0;
    return IsWindow(reinterpret_cast<HWND>(p.source)) &&
           GetWindowThreadProcessId(reinterpret_cast<HWND>(p.source), &pid) && pid==p.pid;
}
LONG State(Packet* p) { return InterlockedCompareExchange(&p->state, Pending, Pending); }
bool OwnsOpen(const Packet& p) {
    return SameSource(p) && reinterpret_cast<std::uintptr_t>(
        GetPropW(reinterpret_cast<HWND>(p.source), kOpenToken))==p.token;
}
bool RestoreOpen(Packet& shared,const Packet* immutable=nullptr,bool synchronize=false) {
    // The source process watchdog and Explorer can finish concurrently. Only
    // one may restore this generation; a failed cross-integrity restore leaves
    // ownership available for the source-process watchdog to retry.
    struct Platform {
        Packet& shared; const Packet& config; bool synchronize;
        bool Claim() { return InterlockedCompareExchange(&shared.restoreClaim,1,0)==0; }
        bool OwnsGeneration() { return OwnsOpen(config); }
        HWND Source() { return reinterpret_cast<HWND>(config.source); }
        void PresentFinalFrame() { if (synchronize) DwmFlush(); }
        bool Uncloak() { BOOL no=FALSE; return SUCCEEDED(SetAttribute(Source(),DWMWA_CLOAK,&no,sizeof(no))); }
        void PresentSource() { if (synchronize) DwmFlush(); }
        bool RestoreTransitions() {
            const BOOL original=TransitionPreference(Source(),config.originalTransitions);
            return SUCCEEDED(SetAttribute(Source(),DWMWA_TRANSITIONS_FORCEDISABLED,&original,sizeof(original)));
        }
        void ReleaseClaim() { InterlockedExchange(&shared.restoreClaim,0); }
        void RemoveToken() { RemovePropW(Source(),kOpenToken); }
    } platform{shared,immutable ? *immutable : shared,synchronize};
    return AppMotion::RestoreOpening(platform);
}

struct Request {
    Handle mapping, event;
    Packet* packet=nullptr;
    HANDLE timer=nullptr, queue=nullptr;
    HWND broker=nullptr;
    std::atomic<bool> finishRequested{false};
    ~Request() { if (packet) UnmapViewOfFile(packet); }
};
std::mutex g_requestsMutex;
std::vector<std::shared_ptr<Request>> g_openRequests;

bool EffectsAllowed() {
    if (g_unloading.load(std::memory_order_relaxed)) return false;
    if (g.respectEffects) {
        BOOL effects=FALSE;
        if (!SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &effects, 0) || !effects) return false;
    }
    if (g.battery) {
        SYSTEM_POWER_STATUS power{};
        if (!GetSystemPowerStatus(&power) || power.ACLineStatus!=1) return false;
    }
    return true;
}

bool Eligible(HWND hwnd, bool hidden,bool restoring=false) {
    DWORD pid=0;
    if (!IsWindow(hwnd) || GetWindowThreadProcessId(hwnd,&pid)!=GetCurrentThreadId() ||
        pid!=GetCurrentProcessId() || GetAncestor(hwnd,GA_ROOT)!=hwnd ||
        GetWindow(hwnd,GW_OWNER) || !IsWindowEnabled(hwnd) || (!restoring && IsIconic(hwnd))) return false;
    const LONG_PTR style=GetWindowLongPtrW(hwnd,GWL_STYLE);
    const LONG_PTR ex=GetWindowLongPtrW(hwnd,GWL_EXSTYLE);
    if ((style&WS_CHILD) || !(style&(WS_CAPTION|WS_THICKFRAME)) ||
        (ex&(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_LAYERED))) return false;
    if (hidden ? !AppMotion::RevealStateMatches(restoring,IsIconic(hwnd)!=FALSE,IsWindowVisible(hwnd)!=FALSE)
               : !IsWindowVisible(hwnd)) return false;
    DWORD affinity=0;
    const bool queried=GetWindowDisplayAffinity(hwnd,&affinity)!=FALSE;
    const auto tracked=g_affinityReliable.load() ?
        reinterpret_cast<std::uintptr_t>(GetPropW(hwnd,kAffinityPreference)) : 0;
    if (!AppMotion::DisplayContentAllowed(!hidden,queried,affinity,tracked!=0,
        tracked ? static_cast<DWORD>(tracked-1) : 0)) {
        Log(L"TRANSITION_NATIVE protected or unknown screen-capture affinity",hwnd); return false;
    }
    DWORD cloaked=0;
    if (FAILED(DwmGetWindowAttribute(hwnd,DWMWA_CLOAKED,&cloaked,sizeof(cloaked))) || cloaked) return false;
    wchar_t cls[100]{};
    if (!GetClassNameW(hwnd,cls,ARRAYSIZE(cls))) return false;
    return wcscmp(cls,L"Shell_TrayWnd") && wcscmp(cls,L"Shell_SecondaryTrayWnd") &&
           wcscmp(cls,L"Progman") && wcscmp(cls,L"WorkerW");
}

HWND Broker() {
    HWND broker=FindWindowW(kBrokerClass,nullptr);
    DWORD brokerPid=0,shellPid=0;
    GetWindowThreadProcessId(broker,&brokerPid);
    GetWindowThreadProcessId(GetShellWindow(),&shellPid);
    return brokerPid && brokerPid==shellPid ? broker : nullptr;
}

struct RestorePacket {
    DWORD magic,version,pid;
    int showCommand;
    std::uint64_t source,token,deadline;
    double queuedMs;
    volatile LONG state;
};
struct RestoreTicket {
    Handle mapping;
    RestorePacket* packet=nullptr;
    RestorePacket config{};
    bool owner=false;
    ~RestoreTicket() {
        if (owner && packet) {
            HWND hwnd=reinterpret_cast<HWND>(config.source);
            if (InterlockedCompareExchange(&packet->state,Pending,Pending)!=Started &&
                reinterpret_cast<std::uintptr_t>(GetPropW(hwnd,kRestorePending))==config.token)
                RemovePropW(hwnd,kRestorePending);
        }
        if (packet) UnmapViewOfFile(packet);
    }
};
std::mutex g_restoresMutex;
std::vector<std::shared_ptr<RestoreTicket>> g_restoreTickets;

bool TicketSource(const RestorePacket& p) {
    DWORD pid=0;
    return GetWindowThreadProcessId(reinterpret_cast<HWND>(p.source),&pid)!=0 && pid==p.pid;
}

bool RestoreShowCommand(int command) { return command==SW_RESTORE || command==SW_SHOWNORMAL || command==SW_SHOWMAXIMIZED; }
bool QueueTaskbarRestore(HWND hwnd,int showCommand=SW_RESTORE) noexcept {
    try {
        // One native SwitchTo invocation may call several restore APIs. Once
        // that invocation has queued a restore, those internal duplicates share
        // the accepted request. A new click gets a fresh scope and native
        // handling when an older click is still pending.
        if (g_taskbarDepth && g_taskbarRoutedWindow==hwnd) return true;
        if (!g.restore || g_unloading) return false;
        if (!IsIconic(hwnd)) { Log(L"TASKBAR_NATIVE target already visible; focus switch is unchanged",hwnd); return false; }
        HWND broker=Broker();
        const bool supported=reinterpret_cast<std::uintptr_t>(GetPropW(hwnd,kClientReady))==kClientVersion;
        const bool pending=GetPropW(hwnd,kRestorePending)!=nullptr;
        const bool hung=IsHungAppWindow(hwnd)!=FALSE;
        if (!AppMotion::CanRouteRestore(true,true,supported,broker!=nullptr,pending,hung) || !EffectsAllowed()) {
            Log(!supported ? L"TASKBAR_NATIVE client not ready; restart target and check CLIENT_READY" :
                !broker ? L"TASKBAR_NATIVE protocol 0.3.1 broker unavailable; re-enable after desktop startup" :
                pending ? L"TASKBAR_NATIVE restore already pending; latest click stays native" :
                hung ? L"TASKBAR_NATIVE target UI reported hung" : L"TASKBAR_NATIVE animation or battery preference disabled effect",hwnd);
            return false;
        }
        DWORD pid=0;
        if (!GetWindowThreadProcessId(hwnd,&pid) || GetWindow(hwnd,GW_OWNER) || !IsWindowEnabled(hwnd)) return false;
        DWORD cloaked=0;
        if (FAILED(DwmGetWindowAttribute(hwnd,DWMWA_CLOAKED,&cloaked,sizeof(cloaked))) || cloaked) return false;
        if (!AllowSetForegroundWindow(pid)) { Log(L"TASKBAR_NATIVE foreground permission unavailable",hwnd); return false; }
        LUID luid{};
        if (!AllocateLocallyUniqueId(&luid)) return false;
        const std::uint64_t token=(static_cast<std::uint64_t>(static_cast<DWORD>(luid.HighPart))<<32)|luid.LowPart;
        auto ticket=std::make_shared<RestoreTicket>();
        auto name=ObjectName(pid,token,L"restore");
        ticket->mapping.value=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,
                                                 sizeof(RestorePacket),name.c_str());
        if (!ticket->mapping.value || GetLastError()==ERROR_ALREADY_EXISTS) return false;
        ticket->packet=static_cast<RestorePacket*>(MapViewOfFile(ticket->mapping.value,FILE_MAP_ALL_ACCESS,0,0,sizeof(RestorePacket)));
        if (!ticket->packet) return false;
        RestorePacket& p=*ticket->packet;
        p={}; p.magic=kMagic; p.version=4; p.pid=pid; p.source=reinterpret_cast<std::uintptr_t>(hwnd);
        p.showCommand=showCommand;
        p.token=token; p.deadline=GetTickCount64()+g.restoreDispatchMs;
        p.queuedMs=PreciseMs();
        ticket->config=p; ticket->owner=true;
        if (!SetPropW(hwnd,kRestorePending,reinterpret_cast<HANDLE>(token))) return false;
        {
            std::lock_guard lock(g_restoresMutex);
            if (g_unloading || g_restoreTickets.size()>=64) return false;
            g_restoreTickets.push_back(ticket);
        }
        InterlockedCompareExchange(&p.state,Pending,Pending);
        if (!PostMessageW(hwnd,g_restoreMessage,static_cast<WPARAM>(token),reinterpret_cast<LPARAM>(broker))) {
            InterlockedExchange(&p.state,Complete);
            PostMessageW(broker,kWake,0,0);
            Log(L"TASKBAR_NATIVE cooperative message submission failed",hwnd);
            return false;
        }
        PostMessageW(broker,kWake,0,0);
        ++g_restoreRouteCount;
        g_taskbarRoutedWindow=hwnd;
        Log(L"TASKBAR_RESTORE_QUEUED no capture or animation-completion wait on taskbar thread",hwnd);
        return true;
    } catch (...) { Log(L"TASKBAR_NATIVE restore preparation failed",hwnd); return false; }
}

void NativeRestoreFallback(RestoreTicket& ticket) {
    if (!AppMotion::ClaimRestoreFallback([&](int desired,int expected) {
        return InterlockedCompareExchange(&ticket.packet->state,desired,expected);
    })) return;
    const RestorePacket& p=ticket.config;
    HWND hwnd=reinterpret_cast<HWND>(p.source);
    if (!TicketSource(p) || !IsIconic(hwnd)) return;
    // Only unclaimed requests take this path. A claimed/completed source
    // restore can never receive a duplicate SW_RESTORE from this watchdog.
    if (g_showAsync) g_showAsync(hwnd,p.showCommand); else ShowWindowAsync(hwnd,p.showCommand);
    SetForegroundWindow(hwnd);
    Log(L"TASKBAR_NATIVE dispatch deadline expired before app claimed request",hwnd);
}

DWORD UpdateRestoreTickets(bool stopping=false) {
    std::array<std::shared_ptr<RestoreTicket>,64> snapshot{};
    size_t count=0;
    { std::lock_guard lock(g_restoresMutex); for (auto& ticket:g_restoreTickets) snapshot[count++]=ticket; }
    DWORD wait=INFINITE;
    const ULONGLONG now=GetTickCount64();
    for (size_t i=0;i<count;++i) {
        auto& ticket=*snapshot[i];
        const LONG state=InterlockedCompareExchange(&ticket.packet->state,Pending,Pending);
        if (!TicketSource(ticket.config)) { InterlockedExchange(&ticket.packet->state,Complete); continue; }
        if (state==Pending && (stopping || now>=ticket.config.deadline)) NativeRestoreFallback(ticket);
        else if (state==Pending) wait=std::min(wait,static_cast<DWORD>(ticket.config.deadline-now));
    }
    {
        std::lock_guard lock(g_restoresMutex);
        std::erase_if(g_restoreTickets,[](const auto& ticket) {
            const LONG state=InterlockedCompareExchange(&ticket->packet->state,Pending,Pending);
            return state==Started || state==Complete || state==Cancelled;
        });
        if (stopping) g_restoreTickets.clear();
    }
    return wait;
}

std::shared_ptr<Request> MakeRequest(HWND hwnd, DWORD operation, std::uint64_t bytes) {
    HWND broker=Broker();
    if (!broker || bytes>32'000'000) return {};
    LUID luid{};
    if (!AllocateLocallyUniqueId(&luid)) return {};
    const std::uint64_t token=(static_cast<std::uint64_t>(static_cast<DWORD>(luid.HighPart))<<32)|luid.LowPart;
    if (!token) return {};
    auto r=std::make_shared<Request>();
    r->broker=broker;
    auto mapName=ObjectName(GetCurrentProcessId(),token,L"pixels");
    auto eventName=ObjectName(GetCurrentProcessId(),token,L"ready");
    r->mapping.value=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,
                                       static_cast<DWORD>(sizeof(Packet)+bytes),mapName.c_str());
    if (!r->mapping.value || GetLastError()==ERROR_ALREADY_EXISTS) return {};
    r->event.value=CreateEventW(nullptr,TRUE,FALSE,eventName.c_str());
    if (!r->event.value || GetLastError()==ERROR_ALREADY_EXISTS) return {};
    r->packet=static_cast<Packet*>(MapViewOfFile(r->mapping.value,FILE_MAP_ALL_ACCESS,0,0,0));
    if (!r->packet) return {};
    Packet& p=*r->packet;
    p={};
    p.magic=kMagic; p.version=4; p.headerBytes=sizeof(Packet); p.operation=operation;
    p.pid=GetCurrentProcessId(); p.source=reinterpret_cast<std::uintptr_t>(hwnd); p.token=token;
    p.durationMs=operation==Opening ? g.openMs : operation==Restoring ? g.restoreMs : g.closeMs;
    p.scalePercent=g.scale;
    p.curve=static_cast<DWORD>(operation==Restoring ? AppMotion::Curve::Out : g.curve);
    p.startOpacityPercent=operation==Restoring ? g.restoreOpacity : 0;
    p.pixelBytes=bytes;
    p.expires=GetTickCount64()+g.handoffMs;
    // TRANSITIONS_FORCEDISABLED is a setter attribute, not a documented getter.
    // Track the app's successful writes instead of depending on an unsupported
    // query. Newly created ordinary windows default to transitions enabled.
    p.originalTransitions=TransitionPreference(hwnd,FALSE);
    return r;
}

struct OcclusionCheck { HWND source; RECT rect; bool blocked; };
BOOL CALLBACK CheckAbove(HWND hwnd, LPARAM param) {
    auto* check=reinterpret_cast<OcclusionCheck*>(param);
    if (hwnd==check->source) return FALSE;
    if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) return TRUE;
    DWORD cloaked=0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd,DWMWA_CLOAKED,&cloaked,sizeof(cloaked))) && cloaked) return TRUE;
    RECT rect{},intersection{};
    if (GetWindowRect(hwnd,&rect) && IntersectRect(&intersection,&rect,&check->rect)) {
        check->blocked=true;
        return FALSE;
    }
    return TRUE;
}

bool Capture(HWND hwnd, Packet* p) {
    DpiScope dpi;
    RECT rect{};
    if (FAILED(DwmGetWindowAttribute(hwnd,DWMWA_EXTENDED_FRAME_BOUNDS,&rect,sizeof(rect)))) return false;
    const int width=rect.right-rect.left,height=rect.bottom-rect.top;
    if (width<32 || height<32 || width>16384 || height>16384) return false;
    RECT desktop{GetSystemMetrics(SM_XVIRTUALSCREEN),GetSystemMetrics(SM_YVIRTUALSCREEN),0,0};
    desktop.right=desktop.left+GetSystemMetrics(SM_CXVIRTUALSCREEN);
    desktop.bottom=desktop.top+GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (rect.left<desktop.left || rect.top<desktop.top || rect.right>desktop.right || rect.bottom>desktop.bottom) return false;
    OcclusionCheck check{hwnd,rect,false};
    EnumWindows(CheckAbove,reinterpret_cast<LPARAM>(&check));
    if (check.blocked) return false;
    const auto size=AppMotion::FitImage(width,height,static_cast<std::uint64_t>(g.megapixels)*1'000'000);
    const auto bytes=static_cast<std::uint64_t>(size.width)*size.height*4;
    if (bytes!=p->pixelBytes) return false;
    BITMAPINFO info{};
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=size.width; info.bmiHeader.biHeight=-size.height;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    HDC screen=GetDC(nullptr);
    if (!screen) return false;
    HDC memory=CreateCompatibleDC(screen);
    void* pixels=nullptr;
    HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
    bool ok=false;
    if (memory && bitmap && pixels) {
        HGDIOBJ old=SelectObject(memory,bitmap);
        SetStretchBltMode(memory,HALFTONE);
        SetBrushOrgEx(memory,0,0,nullptr);
        ok=StretchBlt(memory,0,0,size.width,size.height,screen,rect.left,rect.top,
                      width,height,SRCCOPY|CAPTUREBLT)!=FALSE;
        GdiFlush();
        if (ok) {
            auto* data=static_cast<std::uint32_t*>(pixels);
            for (std::uint64_t i=0;i<bytes/4;++i) data[i]|=0xFF000000;
            // Preserve the small rounded-corner silhouette. Only the corner
            // blocks need alpha work; the interior is copied once unchanged.
            DWORD preference=0;
            DwmGetWindowAttribute(hwnd,static_cast<DWMWINDOWATTRIBUTE>(33),&preference,sizeof(preference));
            const float physicalRadius=IsZoomed(hwnd) || preference==1 ? 0.0f :
                (preference==3 ? 4.0f : 8.0f)*GetDpiForWindow(hwnd)/96.0f;
            const float radius=std::min({physicalRadius*size.width/width,
                                        size.width/2.0f,size.height/2.0f});
            const int edge=static_cast<int>(std::ceil(radius));
            for (int y=0;y<edge;++y) for (int x=0;x<edge;++x) {
                const float distance=std::sqrt((radius-x-0.5f)*(radius-x-0.5f)+
                                                (radius-y-0.5f)*(radius-y-0.5f));
                const unsigned alpha=static_cast<unsigned>(std::lround(
                    255*std::clamp(radius+0.5f-distance,0.0f,1.0f)));
                const int xs[]={x,size.width-1-x},ys[]={y,size.height-1-y};
                for (int yy:ys) for (int xx:xs) {
                    auto& pixel=data[static_cast<size_t>(yy)*size.width+xx];
                    const unsigned b=(pixel&255)*alpha/255;
                    const unsigned green=((pixel>>8)&255)*alpha/255;
                    const unsigned red=((pixel>>16)&255)*alpha/255;
                    pixel=(alpha<<24)|(red<<16)|(green<<8)|b;
                }
            }
            memcpy(reinterpret_cast<BYTE*>(p)+sizeof(Packet),pixels,static_cast<size_t>(bytes));
            p->rect=rect; p->pixelWidth=size.width; p->pixelHeight=size.height;
        }
        SelectObject(memory,old);
    }
    if (bitmap) DeleteObject(bitmap);
    if (memory) DeleteDC(memory);
    ReleaseDC(nullptr,screen);
    return ok;
}

std::shared_ptr<Request> PrepareClose(HWND hwnd) {
    if (!g.close || !EffectsAllowed() || !Eligible(hwnd,false)) return {};
    DpiScope dpi;
    RECT rect{};
    if (FAILED(DwmGetWindowAttribute(hwnd,DWMWA_EXTENDED_FRAME_BOUNDS,&rect,sizeof(rect)))) return {};
    const auto size=AppMotion::FitImage(rect.right-rect.left,rect.bottom-rect.top,
                                       static_cast<std::uint64_t>(g.megapixels)*1'000'000);
    if (!size.width || !size.height) return {};
    auto r=MakeRequest(hwnd,Closing,static_cast<std::uint64_t>(size.width)*size.height*4);
    if (!r || !Capture(hwnd,r->packet)) return {};
    r->packet->expires=GetTickCount64()+g.handoffMs;
    InterlockedCompareExchange(&r->packet->state,Pending,Pending);
    if (!PostMessageW(r->broker,kRequest,r->packet->pid,static_cast<LPARAM>(r->packet->token))) return {};
    WaitForSingleObject(r->event.value,g.handoffMs);
    if (!AppMotion::AcceptOrCancel([&](int desired,int expected) {
            return InterlockedCompareExchange(&r->packet->state,desired,expected);
        })) { Log(L"Close used native fallback (renderer not ready)",hwnd); return {}; }
    BOOL yes=TRUE;
    if (FAILED(SetAttribute(hwnd,DWMWA_TRANSITIONS_FORCEDISABLED,&yes,sizeof(yes)))) {
        InterlockedExchange(&r->packet->state,Cancelled);
        return {};
    }
    return r;
}

void FinishClose(const std::shared_ptr<Request>& r, bool accepted) {
    if (!r) return;
    if (accepted) {
        InterlockedCompareExchange(&r->packet->state,Started,Ready);
        PostMessageW(r->broker,kStartClose,r->packet->pid,static_cast<LPARAM>(r->packet->token));
    } else {
        InterlockedExchange(&r->packet->state,Cancelled);
        if (SameSource(*r->packet)) {
            HWND hwnd=reinterpret_cast<HWND>(r->packet->source);
            const BOOL preference=TransitionPreference(hwnd,r->packet->originalTransitions);
            SetAttribute(hwnd,DWMWA_TRANSITIONS_FORCEDISABLED,&preference,sizeof(BOOL));
        }
    }
}

void CancelOwnOpen(HWND hwnd) {
    const auto token=reinterpret_cast<std::uintptr_t>(GetPropW(hwnd,kOpenToken));
    std::shared_ptr<Request> request;
    {
        std::lock_guard lock(g_requestsMutex);
        for (auto& r:g_openRequests) if (r->packet->source==reinterpret_cast<std::uintptr_t>(hwnd) &&
                                        r->packet->token==token) {
            request=r; break;
        }
    }
    if (!request) return;
    InterlockedExchange(&request->packet->state,Cancelled);
    RestoreOpen(*request->packet);
}

void CALLBACK OpenWatchdog(void* context, BOOLEAN) {
    std::shared_ptr<Request> request;
    {
        std::lock_guard lock(g_requestsMutex);
        auto it=std::find_if(g_openRequests.begin(),g_openRequests.end(),
                            [&](const auto& r){return r.get()==context;});
        if (it==g_openRequests.end()) return;
        request=*it;
        g_openRequests.erase(it);
    }
    const bool finishing=request->finishRequested.load();
    if (!finishing && OwnsOpen(*request->packet)) Log(L"REVEAL_WATCHDOG restoring source visibility after renderer did not finish",reinterpret_cast<HWND>(request->packet->source));
    if (!finishing) InterlockedExchange(&request->packet->state,Cancelled);
    // A renderer that cannot change a foreign window asks this existing
    // one-shot worker to finish locally. DwmFlush never blocks the app UI.
    RestoreOpen(*request->packet,nullptr,finishing);
    if (finishing) InterlockedExchange(&request->packet->state,Complete);
    // Nonblocking deletion from the timer's own callback; context stays alive
    // through the shared_ptr above until the callback returns.
    DeleteTimerQueueTimer(request->queue,request->timer,nullptr);
}

std::shared_ptr<Request> PrepareOpen(HWND hwnd,bool restoring=false) {
    // A framework can hide/show its startup HWND several times. Only the first
    // launch gets this effect; minimized restores have their own path.
    if (!restoring && GetPropW(hwnd,kShownOnce)) return {};
    if (!(restoring ? g.restore : g.open) || !g_timerQueue.load() || !EffectsAllowed() ||
        !Eligible(hwnd,true,restoring)) {
        Log(restoring ? L"RESTORE_NATIVE source reveal eligibility/preferences rejected request" :
                        L"OPEN_NATIVE source reveal eligibility/preferences rejected request",hwnd);
        return {};
    }
    auto r=MakeRequest(hwnd,restoring ? Restoring : Opening,0);
    if (!r) { Log(L"REVEAL_NATIVE renderer or request mapping unavailable",hwnd); return {}; }
    struct Recovery {
        Request* request; bool committed=false;
        ~Recovery() {
            if (!committed) {
                InterlockedExchange(&request->packet->state,Cancelled);
                RestoreOpen(*request->packet);
            }
        }
    } recovery{r.get()};
    if (!SetPropW(hwnd,kOpenToken,reinterpret_cast<HANDLE>(r->packet->token))) return {};
    {
        std::lock_guard lock(g_requestsMutex);
        r->queue=g_timerQueue.load();
        if (g_unloading || !r->queue) return {};
        g_openRequests.push_back(r);
        if (!CreateTimerQueueTimer(&r->timer,r->queue,OpenWatchdog,r.get(),
                                   static_cast<DWORD>((restoring ? g.restoreMs : g.openMs)+g.handoffMs+300),0,WT_EXECUTEONLYONCE)) {
            g_openRequests.pop_back();
            return {};
        }
    }
    // Install recovery before any concealment. If DWM stalls long enough for
    // that watchdog to fire, do not start a new concealment afterward.
    if (State(r->packet)!=Pending || !OwnsOpen(*r->packet)) return {};
    BOOL yes=TRUE;
    if (FAILED(SetAttribute(hwnd,DWMWA_TRANSITIONS_FORCEDISABLED,&yes,sizeof(yes))) ||
        FAILED(SetAttribute(hwnd,DWMWA_CLOAK,&yes,sizeof(yes)))) return {};
    if (State(r->packet)!=Pending || !OwnsOpen(*r->packet)) {
        BOOL no=FALSE;
        SetAttribute(hwnd,DWMWA_CLOAK,&no,sizeof(no));
        const BOOL original=TransitionPreference(hwnd,r->packet->originalTransitions);
        SetAttribute(hwnd,DWMWA_TRANSITIONS_FORCEDISABLED,&original,sizeof(original));
        return {};
    }
    recovery.committed=true;
    return r;
}

std::shared_ptr<Request> SafePrepare(HWND hwnd,bool opening,bool restoring=false) noexcept {
    try { return opening ? PrepareOpen(hwnd,restoring) : PrepareClose(hwnd); }
    catch (...) { Log(L"Preparation failed; using native operation",hwnd); return {}; }
}

void SubmitOpen(const std::shared_ptr<Request>& r) {
    if (!r) return;
    Packet& p=*r->packet;
    HWND hwnd=reinterpret_cast<HWND>(p.source);
    DpiScope dpi;
    if (!OwnsOpen(p) || !IsWindowVisible(hwnd) || IsIconic(hwnd) ||
        FAILED(DwmGetWindowAttribute(hwnd,DWMWA_EXTENDED_FRAME_BOUNDS,&p.rect,sizeof(p.rect)))) {
        InterlockedExchange(&p.state,Cancelled); RestoreOpen(p); return;
    }
    p.expires=GetTickCount64()+g.handoffMs+100;
    if (InterlockedCompareExchange(&p.state,Pending,Pending)!=Pending) { RestoreOpen(p); return; }
    if (!PostMessageW(r->broker,kRequest,p.pid,static_cast<LPARAM>(p.token))) {
        InterlockedExchange(&p.state,Cancelled); RestoreOpen(p);
    } else if (p.operation==Opening) {
        SetPropW(hwnd,kShownOnce,reinterpret_cast<HANDLE>(1));
    }
}

BOOL WINAPI DestroyHook(HWND hwnd) {
    const DWORD entryError=GetLastError(); ScopeDepth depth;
    if (g_hookDepth!=1 || g_unloading) { SetLastError(entryError); return g_destroy(hwnd); }
    const bool opening=GetPropW(hwnd,kOpenToken)!=nullptr;
    if (opening) CancelOwnOpen(hwnd);
    auto close=opening ? std::shared_ptr<Request>{} : SafePrepare(hwnd,false);
    SetLastError(entryError);
    const BOOL result=g_destroy(hwnd);
    const DWORD nativeError=GetLastError();
    FinishClose(close,result || !IsWindow(hwnd));
    SetLastError(nativeError); return result;
}

bool ShowIntent(int command) {
    return command==SW_SHOW || command==SW_SHOWNORMAL || command==SW_SHOWMAXIMIZED ||
           command==SW_SHOWDEFAULT || command==SW_RESTORE;
}
bool ShowKeepsReveal(HWND hwnd,int command) {
    return command==SW_SHOW || command==SW_SHOWNA || command==SW_SHOWNOACTIVATE ||
           ((command==SW_SHOWNORMAL || command==SW_SHOWDEFAULT) && !IsZoomed(hwnd) && !IsIconic(hwnd));
}
BOOL WINAPI ShowHook(HWND hwnd,int command) {
    const DWORD entryError=GetLastError(); ScopeDepth depth;
    if (g_hookDepth!=1 || g_unloading) { SetLastError(entryError); return g_show(hwnd,command); }
    if (g_taskbarDepth && RestoreShowCommand(command)) {
        const BOOL wasVisible=IsWindowVisible(hwnd);
        if (QueueTaskbarRestore(hwnd,command)) { SetLastError(entryError); return wasVisible; }
    }
    const bool wasOpening=GetPropW(hwnd,kOpenToken)!=nullptr;
    // Redundant activation/show calls are common during startup. They must
    // not reveal the full-size source underneath a half-faded thumbnail.
    if (wasOpening && !ShowKeepsReveal(hwnd,command)) CancelOwnOpen(hwnd);
    auto open=ShowIntent(command) && !wasOpening ? SafePrepare(hwnd,true,IsIconic(hwnd)!=FALSE) : std::shared_ptr<Request>{};
    auto close=command==SW_HIDE && g.hides && !wasOpening ? SafePrepare(hwnd,false) : std::shared_ptr<Request>{};
    SetLastError(entryError);
    const BOOL result=g_show(hwnd,command);
    const DWORD nativeError=GetLastError();
    SubmitOpen(open); FinishClose(close,!IsWindowVisible(hwnd));
    if (close && IsWindow(hwnd)) {
        const BOOL preference=TransitionPreference(hwnd,close->packet->originalTransitions);
        SetAttribute(hwnd,DWMWA_TRANSITIONS_FORCEDISABLED,&preference,sizeof(BOOL));
    }
    SetLastError(nativeError); return result;
}

BOOL WINAPI ShowAsyncHook(HWND hwnd,int command) {
    const DWORD error=GetLastError(); ScopeDepth depth;
    if (g_hookDepth==1 && !g_unloading && g_taskbarDepth && RestoreShowCommand(command) && QueueTaskbarRestore(hwnd,command)) {
        SetLastError(error); return TRUE;
    }
    SetLastError(error); return g_showAsync(hwnd,command);
}
void WINAPI SwitchWindowHook(HWND hwnd,BOOL altTab) {
    const DWORD error=GetLastError(); ScopeDepth depth;
    if (g_hookDepth==1 && !g_unloading && g_taskbarDepth && QueueTaskbarRestore(hwnd)) {
        SetLastError(error); return;
    }
    SetLastError(error); g_switchWindow(hwnd,altTab);
}

void MarkRestoreClient(HWND hwnd) {
    if (!g.restore || g_unloading || !g_restoreMessage || !hwnd) return;
    hwnd=GetAncestor(hwnd,GA_ROOT);
    DWORD pid=0;
    if (!hwnd || GetWindowThreadProcessId(hwnd,&pid)!=GetCurrentThreadId() ||
        pid!=GetCurrentProcessId() || GetWindow(hwnd,GW_OWNER)) return;
    if (reinterpret_cast<std::uintptr_t>(GetPropW(hwnd,kClientReady))==kClientVersion) return;
    const LONG_PTR style=GetWindowLongPtrW(hwnd,GWL_STYLE);
    const LONG_PTR ex=GetWindowLongPtrW(hwnd,GWL_EXSTYLE);
    if (!(style&(WS_CAPTION|WS_THICKFRAME)) || (style&WS_CHILD) ||
        (ex&(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE))) return;
    if (SetPropW(hwnd,kClientReady,reinterpret_cast<HANDLE>(kClientVersion)))
        Log(L"CLIENT_READY source UI dispatcher supports cooperative restore",hwnd);
}

bool HandleRestoreMessage(const MSG& message) {
    if (!g_restoreMessage || message.message!=g_restoreMessage || !message.hwnd ||
        reinterpret_cast<HWND>(message.lParam)!=Broker()) return false;
    const std::uint64_t token=static_cast<std::uint64_t>(message.wParam);
    Handle mapping(OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,
        ObjectName(GetCurrentProcessId(),token,L"restore").c_str()));
    if (!mapping.value) return true;
    auto* p=static_cast<RestorePacket*>(MapViewOfFile(mapping.value,FILE_MAP_ALL_ACCESS,0,0,sizeof(RestorePacket)));
    if (!p) return true;
    struct Finish {
        RestorePacket* packet; bool claimed=false;
        RestorePacket config{};
        ~Finish() {
            if (claimed) {
                HWND hwnd=reinterpret_cast<HWND>(config.source);
                if (reinterpret_cast<std::uintptr_t>(GetPropW(hwnd,kRestorePending))==config.token)
                    RemovePropW(hwnd,kRestorePending);
                InterlockedExchange(&packet->state,Complete);
            }
            UnmapViewOfFile(packet);
        }
    } finish{p,false,{}};
    if (InterlockedCompareExchange(&p->state,Pending,Pending)!=Pending) return true;
    const RestorePacket copy=*p;
    if (copy.magic!=kMagic || copy.version!=4 || copy.pid!=GetCurrentProcessId() ||
        copy.token!=token || reinterpret_cast<HWND>(copy.source)!=message.hwnd ||
        !RestoreShowCommand(copy.showCommand) || !TicketSource(copy)) return true;
    finish.config=copy;
    if (!AppMotion::ClaimRestore([&](int desired,int expected) {
        return InterlockedCompareExchange(&p->state,desired,expected);
    })) return true;
    finish.claimed=true;
    ScopeDepth depth;
    HWND hwnd=message.hwnd;
    if (!IsIconic(hwnd)) { Log(L"RESTORE_NATIVE latest input already restored window; obsolete request skipped",hwnd); return true; }
    if (g.diagnostics) Wh_Log(L"RESTORE_RECEIVED dispatch_ms=%.2f hwnd=%p",PreciseMs()-copy.queuedMs,hwnd);
    auto animation=g_unloading ? std::shared_ptr<Request>{} : SafePrepare(hwnd,true,true);
    // Recheck after preparation: SW_RESTORE on an already restored maximized
    // window can unmaximize it. Never replay that operation after newer input.
    if (IsIconic(hwnd)) g_show(hwnd,copy.showCommand);
    else if (animation) { InterlockedExchange(&animation->packet->state,Cancelled); RestoreOpen(*animation->packet); animation.reset(); }
    if (!SetForegroundWindow(hwnd)) Log(L"RESTORE_FOCUS native foreground policy rejected activation",hwnd);
    SubmitOpen(animation);
    return true;
}

bool DispatchRestore(const MSG* message) noexcept {
    if (!message) return false;
    try {
        if (g_finishRevealMessage && message->message==g_finishRevealMessage) {
            if (reinterpret_cast<HWND>(message->lParam)!=Broker()) return true;
            std::lock_guard lock(g_requestsMutex);
            for (const auto& r:g_openRequests) {
                if (r->packet->source==reinterpret_cast<std::uintptr_t>(message->hwnd) &&
                    r->packet->token==static_cast<std::uint64_t>(message->wParam) &&
                    OwnsOpen(*r->packet)) {
                    r->finishRequested.store(true);
                    ChangeTimerQueueTimer(r->queue,r->timer,0,0);
                    break;
                }
            }
            return true;
        }
        if (g.restore && !g_unloading && !g_clientDispatcherSeen) {
            g_clientDispatcherSeen=true;
            // Show/activate messages are often sent directly to WndProc. Do
            // not require those messages to pass through DispatchMessage before
            // declaring a normal app ready. Observe the real UI dispatcher once
            // and enumerate that thread's existing top-level windows.
            EnumThreadWindows(GetCurrentThreadId(),[](HWND hwnd,LPARAM)->BOOL {
                MarkRestoreClient(hwnd); return TRUE;
            },0);
        }
        if (message->message==g_restoreMessage && g_restoreMessage) return HandleRestoreMessage(*message);
        if (message->message==WM_PAINT || message->message==WM_SHOWWINDOW ||
            message->message==WM_ACTIVATE || message->message==WM_WINDOWPOSCHANGED)
            MarkRestoreClient(message->hwnd);
    } catch (...) { Log(L"RESTORE_NATIVE source dispatcher failed; request watchdog can take pending work",message->hwnd); }
    return false;
}
LRESULT WINAPI DispatchWHook(const MSG* message) {
    const DWORD error=GetLastError();
    if (DispatchRestore(message)) { SetLastError(error); return 0; }
    SetLastError(error); return g_dispatchW(message);
}
LRESULT WINAPI DispatchAHook(const MSG* message) {
    const DWORD error=GetLastError();
    if (DispatchRestore(message)) { SetLastError(error); return 0; }
    SetLastError(error); return g_dispatchA(message);
}
BOOL WINAPI PositionHook(HWND hwnd,HWND after,int x,int y,int cx,int cy,UINT flags) {
    const DWORD entryError=GetLastError(); ScopeDepth depth;
    if (g_hookDepth!=1 || g_unloading) { SetLastError(entryError); return g_position(hwnd,after,x,y,cx,cy,flags); }
    const bool wasOpening=GetPropW(hwnd,kOpenToken)!=nullptr;
    bool geometry=false;
    if (wasOpening && (flags&(SWP_NOMOVE|SWP_NOSIZE))!=(SWP_NOMOVE|SWP_NOSIZE)) {
        // Use the caller's DPI context, the same one used by SetWindowPos.
        RECT before{};
        geometry=!GetWindowRect(hwnd,&before) || AppMotion::ChangesGeometry(
            {before.left,before.top,before.right,before.bottom},
            x,y,cx,cy,
            !(flags&SWP_NOMOVE),!(flags&SWP_NOSIZE));
    }
    if (wasOpening && (geometry || (flags&(SWP_HIDEWINDOW|SWP_FRAMECHANGED)))) CancelOwnOpen(hwnd);
    auto open=(flags&SWP_SHOWWINDOW) && !(flags&(SWP_HIDEWINDOW|SWP_NOACTIVATE)) && !wasOpening ?
               SafePrepare(hwnd,true) : std::shared_ptr<Request>{};
    auto close=(flags&SWP_HIDEWINDOW) && g.hides && !wasOpening ? SafePrepare(hwnd,false) : std::shared_ptr<Request>{};
    SetLastError(entryError);
    const BOOL result=g_position(hwnd,after,x,y,cx,cy,flags);
    const DWORD nativeError=GetLastError();
    if (result) { SubmitOpen(open); FinishClose(close,!IsWindowVisible(hwnd)); }
    else { if (open) { InterlockedExchange(&open->packet->state,Cancelled); RestoreOpen(*open->packet); } FinishClose(close,false); }
    if (close && IsWindow(hwnd)) {
        const BOOL preference=TransitionPreference(hwnd,close->packet->originalTransitions);
        SetAttribute(hwnd,DWMWA_TRANSITIONS_FORCEDISABLED,&preference,sizeof(BOOL));
    }
    SetLastError(nativeError); return result;
}

// All following rendering objects are confined to the Explorer broker thread.
struct Graphics {
    HMODULE d3dLibrary=nullptr,d2dLibrary=nullptr,compositionLibrary=nullptr;
    ComPtr<ID3D11Device> d3d;
    ComPtr<IDCompositionDevice> composition;
    ComPtr<ID2D1DeviceContext> drawing;
    ~Graphics() { Reset(); }
    void Reset() {
        drawing.Reset(); composition.Reset(); d3d.Reset();
        if (compositionLibrary) { FreeLibrary(compositionLibrary); compositionLibrary=nullptr; }
        if (d2dLibrary) { FreeLibrary(d2dLibrary); d2dLibrary=nullptr; }
        if (d3dLibrary) { FreeLibrary(d3dLibrary); d3dLibrary=nullptr; }
    }
    bool Init() {
        if (composition) return true;
        // The injected app-side DLL must not load the full graphics stack in
        // every process. Resolve these APIs only on the Explorer render thread.
        d3dLibrary=LoadLibraryExW(L"d3d11.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        d2dLibrary=LoadLibraryExW(L"d2d1.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        compositionLibrary=LoadLibraryExW(L"dcomp.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!d3dLibrary || !d2dLibrary || !compositionLibrary) { Reset(); return false; }
        const auto createD3D=reinterpret_cast<decltype(&D3D11CreateDevice)>(GetProcAddress(d3dLibrary,"D3D11CreateDevice"));
        using FactoryFn=HRESULT(WINAPI*)(D2D1_FACTORY_TYPE,REFIID,const D2D1_FACTORY_OPTIONS*,void**);
        const auto createFactory=reinterpret_cast<FactoryFn>(GetProcAddress(d2dLibrary,"D2D1CreateFactory"));
        const auto createComposition=reinterpret_cast<decltype(&DCompositionCreateDevice)>(
            GetProcAddress(compositionLibrary,"DCompositionCreateDevice"));
        if (!createD3D || !createFactory || !createComposition) { Reset(); return false; }
        D3D_FEATURE_LEVEL level{};
        if (FAILED(createD3D(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&d3d,&level,nullptr))) { Reset(); return false; }
        ComPtr<IDXGIDevice> dxgi;
        ComPtr<ID2D1Device> d2d;
        ComPtr<ID2D1Factory1> factory;
        if (FAILED(d3d.As(&dxgi)) || FAILED(createFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
            __uuidof(ID2D1Factory1),nullptr,reinterpret_cast<void**>(factory.GetAddressOf()))) ||
            FAILED(factory->CreateDevice(dxgi.Get(),&d2d)) ||
            FAILED(d2d->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,&drawing)) ||
            FAILED(createComposition(dxgi.Get(),__uuidof(IDCompositionDevice),
                reinterpret_cast<void**>(composition.GetAddressOf())))) {
            Reset(); return false;
        }
        return true;
    }
};
struct Animation {
    Handle mapping,event;
    Packet* packet=nullptr;
    Packet config{};
    HWND overlay=nullptr;
    HTHUMBNAIL thumbnail=nullptr;
    ComPtr<IDCompositionTarget> target;
    ComPtr<IDCompositionVisual> root,content;
    ComPtr<IDCompositionScaleTransform> scale;
    ComPtr<IDCompositionEffectGroup> effects;
    ComPtr<IDCompositionSurface> surface;
    ULONGLONG started=0, deadline=0;
    double startedPrecise=0;
    bool running=false,finishing=false;
    ~Animation() {
        if (packet && LiveOperation(config.operation)) RestoreOpen(*packet,&config);
        if (thumbnail) DwmUnregisterThumbnail(thumbnail);
        if (overlay) DestroyWindow(overlay);
        if (packet) { InterlockedExchange(&packet->state,Complete); UnmapViewOfFile(packet); }
    }
};
struct BrokerState {
    Graphics graphics;
    std::vector<std::unique_ptr<Animation>> animations;
} *g_broker=nullptr;

LRESULT CALLBACK OverlayProc(HWND hwnd,UINT message,WPARAM w,LPARAM l) {
    if (message==WM_NCHITTEST) return HTTRANSPARENT;
    if (message==WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    if (message==WM_ERASEBKGND) return 1;
    return DefWindowProcW(hwnd,message,w,l);
}

bool Upload(Animation& a,Graphics& gfx) {
    Packet& p=a.config;
    if (!gfx.Init()) return false;
    auto device=gfx.composition.Get();
    if (FAILED(device->CreateTargetForHwnd(a.overlay,TRUE,&a.target)) ||
        FAILED(device->CreateVisual(&a.root)) || FAILED(device->CreateVisual(&a.content)) ||
        FAILED(device->CreateScaleTransform(&a.scale)) ||
        FAILED(device->CreateEffectGroup(&a.effects)) ||
        FAILED(device->CreateSurface(p.pixelWidth,p.pixelHeight,DXGI_FORMAT_B8G8R8A8_UNORM,
                                      DXGI_ALPHA_MODE_PREMULTIPLIED,&a.surface))) return false;
    ComPtr<IDXGISurface> dxgi;
    POINT offset{};
    if (FAILED(a.surface->BeginDraw(nullptr,__uuidof(IDXGISurface),
                 reinterpret_cast<void**>(dxgi.GetAddressOf()),&offset))) return false;
    ComPtr<ID2D1Bitmap1> destination,bitmap;
    D2D1_BITMAP_PROPERTIES1 properties{};
    properties.pixelFormat={DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED};
    properties.dpiX=96; properties.dpiY=96;
    properties.bitmapOptions=D2D1_BITMAP_OPTIONS_TARGET|D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    HRESULT hr=gfx.drawing->CreateBitmapFromDxgiSurface(dxgi.Get(),&properties,&destination);
    properties.bitmapOptions=D2D1_BITMAP_OPTIONS_NONE;
    if (SUCCEEDED(hr)) hr=gfx.drawing->CreateBitmap(D2D1::SizeU(p.pixelWidth,p.pixelHeight),
        reinterpret_cast<BYTE*>(a.packet)+sizeof(Packet),p.pixelWidth*4,&properties,&bitmap);
    if (SUCCEEDED(hr)) {
        gfx.drawing->SetTarget(destination.Get());
        gfx.drawing->BeginDraw();
        gfx.drawing->Clear(D2D1::ColorF(0,0));
        gfx.drawing->DrawBitmap(bitmap.Get(),D2D1::RectF(static_cast<float>(offset.x),
            static_cast<float>(offset.y),static_cast<float>(offset.x+p.pixelWidth),
            static_cast<float>(offset.y+p.pixelHeight)),1,D2D1_INTERPOLATION_MODE_LINEAR);
        hr=gfx.drawing->EndDraw();
        gfx.drawing->SetTarget(nullptr);
    }
    const HRESULT end=a.surface->EndDraw();
    if (FAILED(hr) || FAILED(end)) return false;
    ComPtr<IDCompositionScaleTransform> fit;
    const float width=static_cast<float>(p.rect.right-p.rect.left);
    const float height=static_cast<float>(p.rect.bottom-p.rect.top);
    if (FAILED(device->CreateScaleTransform(&fit)) || FAILED(fit->SetScaleX(width/p.pixelWidth)) ||
        FAILED(fit->SetScaleY(height/p.pixelHeight)) || FAILED(a.content->SetContent(a.surface.Get())) ||
        FAILED(a.content->SetTransform(fit.Get())) || FAILED(a.root->AddVisual(a.content.Get(),FALSE,nullptr)) ||
        FAILED(a.scale->SetCenterX(width/2)) || FAILED(a.scale->SetCenterY(height/2)) ||
        FAILED(a.scale->SetScaleX(1.0f)) || FAILED(a.scale->SetScaleY(1.0f)) ||
        FAILED(a.root->SetTransform(a.scale.Get())) || FAILED(a.effects->SetOpacity(1.0f)) ||
        FAILED(a.root->SetEffect(a.effects.Get())) ||
        FAILED(a.target->SetRoot(a.root.Get())) || FAILED(device->Commit())) return false;
    return true;
}

bool StartClosing(Animation& a) {
    Packet& p=a.config;
    auto* device=g_broker->graphics.composition.Get();
    auto animation=[&](float from,float to,ComPtr<IDCompositionAnimation>& output) {
        const float seconds=p.durationMs/1000.0f;
        const auto c=AppMotion::Coefficients(from,to,seconds,static_cast<AppMotion::Curve>(p.curve));
        return SUCCEEDED(device->CreateAnimation(&output)) &&
               SUCCEEDED(output->AddCubic(0,c.a,c.b,c.c,c.d)) &&
               SUCCEEDED(output->End(seconds,to));
    };
    ComPtr<IDCompositionAnimation> opacity,scale;
    if (!device || !animation(1,0,opacity) || !animation(1,p.scalePercent/100.0f,scale) ||
        FAILED(a.effects->SetOpacity(opacity.Get())) || FAILED(a.scale->SetScaleX(scale.Get())) ||
        FAILED(a.scale->SetScaleY(scale.Get())) || FAILED(device->Commit())) return false;
    a.started=GetTickCount64(); a.deadline=a.started+p.durationMs+34; a.running=true;
    Log(L"GPU close started",reinterpret_cast<HWND>(p.source));
    return true;
}

std::unique_ptr<Animation> ReadRequest(DWORD pid,std::uint64_t token) {
    auto a=std::make_unique<Animation>();
    a->mapping.value=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,ObjectName(pid,token,L"pixels").c_str());
    if (!a->mapping.value) return {};
    auto* header=static_cast<Packet*>(MapViewOfFile(a->mapping.value,FILE_MAP_ALL_ACCESS,0,0,sizeof(Packet)));
    if (!header) return {};
    if (State(header)!=Pending) { UnmapViewOfFile(header); return {}; }
    Packet copy=*header;
    UnmapViewOfFile(header);
    const std::int64_t width=static_cast<std::int64_t>(copy.rect.right)-copy.rect.left;
    const std::int64_t height=static_cast<std::int64_t>(copy.rect.bottom)-copy.rect.top;
    if (copy.magic!=kMagic || copy.version!=4 || copy.headerBytes!=sizeof(Packet) ||
        copy.pid!=pid || copy.token!=token || (!LiveOperation(copy.operation) && copy.operation!=Closing) ||
        copy.durationMs<60 || copy.durationMs>400 || copy.scalePercent<90 || copy.scalePercent>100 ||
        copy.curve>2 || copy.startOpacityPercent>100 || width<32 || height<32 || width>16384 || height>16384 ||
        copy.expires<GetTickCount64() || (LiveOperation(copy.operation) ? copy.pixelBytes!=0 :
        !AppMotion::ValidImage(copy.pixelWidth,copy.pixelHeight,copy.pixelBytes))) return {};
    a->config=copy;
    a->packet=static_cast<Packet*>(MapViewOfFile(a->mapping.value,FILE_MAP_ALL_ACCESS,0,0,
                                               sizeof(Packet)+static_cast<SIZE_T>(copy.pixelBytes)));
    if (!a->packet) return {};
    if (State(a->packet)!=Pending || !SameSource(copy)) return {};
    a->event.value=OpenEventW(EVENT_MODIFY_STATE,FALSE,ObjectName(pid,token,L"ready").c_str());
    if (!a->event.value) return {};
    return a;
}

void ReceiveRequest(DWORD pid,std::uint64_t token) {
    // Duplicate/replayed messages must not create a second overlay for a token.
    for (const auto& a:g_broker->animations) if (a->config.pid==pid && a->config.token==token) return;
    auto a=ReadRequest(pid,token);
    if (!a) { if (g.diagnostics) Wh_Log(L"RENDERER_NATIVE request could not be opened or validated pid=%lu token=%llX",pid,static_cast<unsigned long long>(token)); return; }
    Packet& p=a->config;
    if (g_broker->animations.size()>=static_cast<size_t>(g.concurrent)) {
        Log(L"RENDERER_NATIVE active-animation budget reached",reinterpret_cast<HWND>(p.source)); return;
    }
    HWND source=reinterpret_cast<HWND>(p.source);
    if (p.operation==Restoring && GetAncestor(GetForegroundWindow(),GA_ROOT)!=source) {
        Log(L"RESTORE_NATIVE newer activation or foreground policy takes priority",source); return;
    }
    if (p.operation==Closing) {
        RECT current{};
        if (FAILED(DwmGetWindowAttribute(source,DWMWA_EXTENDED_FRAME_BOUNDS,&current,sizeof(current))) ||
            !EqualRect(&current,&p.rect)) return;
    }
    const int width=p.rect.right-p.rect.left,height=p.rect.bottom-p.rect.top;
    DWORD ex=WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TRANSPARENT|WS_EX_NOREDIRECTIONBITMAP;
    const bool topmost=(GetWindowLongPtrW(source,GWL_EXSTYLE)&WS_EX_TOPMOST)!=0;
    if (topmost) ex|=WS_EX_TOPMOST;
    if (LiveOperation(p.operation)) ex|=WS_EX_LAYERED;
    a->overlay=CreateWindowExW(ex,kOverlayClass,L"",WS_POPUP,p.rect.left,p.rect.top,width,height,
                              nullptr,nullptr,g_module,nullptr);
    if (!a->overlay) return;
    if (LiveOperation(p.operation)) {
        if (!OwnsOpen(p)) { Log(L"RENDERER_NATIVE reveal cancelled or source no longer owned",source); return; }
        const HRESULT registered=DwmRegisterThumbnail(a->overlay,source,&a->thumbnail);
        if (FAILED(registered)) { if (g.diagnostics) Wh_Log(L"RENDERER_NATIVE thumbnail registration hr=0x%08lX hwnd=%p",registered,source); return; }
        DWM_THUMBNAIL_PROPERTIES thumb{};
        thumb.dwFlags=DWM_TNP_VISIBLE|DWM_TNP_OPACITY|DWM_TNP_RECTDESTINATION|DWM_TNP_SOURCECLIENTAREAONLY;
        thumb.fVisible=TRUE; thumb.opacity=static_cast<BYTE>(std::lround(255*p.startOpacityPercent/100.0f));
        thumb.fSourceClientAreaOnly=FALSE;
        const int w=static_cast<int>(std::lround(width*p.scalePercent/100.0f));
        const int h=static_cast<int>(std::lround(height*p.scalePercent/100.0f));
        thumb.rcDestination={(width-w)/2,(height-h)/2,(width+w)/2,(height+h)/2};
        if (FAILED(DwmUpdateThumbnailProperties(a->thumbnail,&thumb))) return;
    } else if (!Upload(*a,g_broker->graphics)) { Log(L"CLOSE_NATIVE GPU device/upload unavailable",source); return; }
    if (State(a->packet)!=Pending || p.expires<GetTickCount64()) return;
    HWND previous=GetWindow(source,GW_HWNDPREV);
    if (!previous || ((GetWindowLongPtrW(previous,GWL_EXSTYLE)&WS_EX_TOPMOST)!=0)!=topmost)
        previous=topmost ? HWND_TOPMOST : HWND_TOP;
    if (!SetWindowPos(a->overlay,previous,p.rect.left,p.rect.top,width,height,
                      SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_NOOWNERZORDER)) {
        Log(L"RENDERER_NATIVE overlay could not be shown",source); return;
    }
    // Only the broker waits for presentation. The app's close handoff has its
    // own bounded deadline, and its open path does not wait for this thread.
    if (FAILED(DwmFlush())) return;
    if (!AppMotion::PublishReady([&](int desired,int expected) {
            return InterlockedCompareExchange(&a->packet->state,desired,expected);
        })) return;
    a->started=GetTickCount64(); a->deadline=a->started+(LiveOperation(p.operation) ? p.durationMs+150 : 250);
    a->startedPrecise=PreciseMs();
    if (LiveOperation(p.operation)) {
        a->running=true;
        Log(p.operation==Restoring ? L"RESTORE_ANIMATION_STARTED GPU live thumbnail" : L"OPEN_ANIMATION_STARTED GPU live thumbnail",source);
    }
    g_broker->animations.push_back(std::move(a));
    SetEvent(g_broker->animations.back()->event.value);
}

LRESULT CALLBACK BrokerProc(HWND hwnd,UINT message,WPARAM w,LPARAM l) {
    if (message==kWake) return 0;
    if (message==kRequest && g_broker) {
        try { ReceiveRequest(static_cast<DWORD>(w),static_cast<std::uint64_t>(l)); }
        catch (...) { if (g.diagnostics) Wh_Log(L"Renderer preparation failed; native/watchdog fallback remains available."); }
        return 0;
    }
    if (message==kStartClose && g_broker) {
        for (auto& a:g_broker->animations) if (a->config.pid==static_cast<DWORD>(w) &&
            a->config.token==static_cast<std::uint64_t>(l) && a->config.operation==Closing && !a->running) {
            if (State(a->packet)==Started && !StartClosing(*a)) InterlockedExchange(&a->packet->state,Cancelled);
        }
        return 0;
    }
    return DefWindowProcW(hwnd,message,w,l);
}

bool UpdateAnimations() {
    const ULONGLONG now=GetTickCount64();
    const double precise=PreciseMs();
    bool live=false;
    auto& list=g_broker->animations;
    for (auto& a:list) {
        Packet& p=a->config;
        LONG state=State(a->packet);
        if (state==Cancelled || state==Complete) continue;
        if (LiveOperation(p.operation)) {
            if (a->finishing) {
                if (!OwnsOpen(p)) {
                    DwmFlush();
                    InterlockedExchange(&a->packet->state,Complete);
                } else if (now>=a->deadline) {
                    InterlockedExchange(&a->packet->state,Cancelled);
                } else live=true;
                continue;
            }
            if (!OwnsOpen(p) || !IsWindowVisible(reinterpret_cast<HWND>(p.source)) ||
                IsIconic(reinterpret_cast<HWND>(p.source)) ||
                (p.operation==Restoring && GetAncestor(GetForegroundWindow(),GA_ROOT)!=reinterpret_cast<HWND>(p.source))) {
                InterlockedExchange(&a->packet->state,Cancelled); continue;
            }
            const float t=std::min(1.0f,static_cast<float>((precise-a->startedPrecise)/p.durationMs));
            const float progress=AppMotion::Ease(t,static_cast<AppMotion::Curve>(p.curve));
            const float scale=p.scalePercent/100.0f+(1-p.scalePercent/100.0f)*progress;
            const int width=p.rect.right-p.rect.left,height=p.rect.bottom-p.rect.top;
            const int scaledW=static_cast<int>(std::lround(width*scale));
            const int scaledH=static_cast<int>(std::lround(height*scale));
            DWM_THUMBNAIL_PROPERTIES thumb{};
            thumb.dwFlags=DWM_TNP_RECTDESTINATION|DWM_TNP_OPACITY;
            thumb.opacity=static_cast<BYTE>(std::lround(255*(p.startOpacityPercent/100.0f+
                                       (1-p.startOpacityPercent/100.0f)*progress)));
            thumb.rcDestination={(width-scaledW)/2,(height-scaledH)/2,
                                  (width+scaledW)/2,(height+scaledH)/2};
            if (FAILED(DwmUpdateThumbnailProperties(a->thumbnail,&thumb))) { InterlockedExchange(&a->packet->state,Cancelled); continue; }
            if (t>=1) {
                // Flush the final thumbnail, uncloak under its opaque cover,
                // then flush the source BEFORE destroying the overlay or
                // re-enabling the native transition preference. Broker only:
                // no compositor-completion wait is added to the app UI thread.
                RestoreOpen(*a->packet,&p,true);
                if (!OwnsOpen(p)) InterlockedExchange(&a->packet->state,Complete);
                else {
                    a->finishing=true;
                    PostMessageW(reinterpret_cast<HWND>(p.source),g_finishRevealMessage,
                        static_cast<WPARAM>(p.token),reinterpret_cast<LPARAM>(Broker()));
                    live=true;
                    Log(L"REVEAL_HANDOFF requesting source-process recovery worker",reinterpret_cast<HWND>(p.source));
                }
            }
            else live=true;
        } else if (!a->running && (state==Started || !SameSource(p) ||
                                   !IsWindowVisible(reinterpret_cast<HWND>(p.source)))) {
            if (!StartClosing(*a)) InterlockedExchange(&a->packet->state,Cancelled);
        }
        if (now>=a->deadline) InterlockedExchange(&a->packet->state,Complete);
    }
    std::erase_if(list,[](const auto& a){const LONG state=State(a->packet); return state==Cancelled || state==Complete;});
    return live;
}

DWORD WINAPI BrokerThread(void*) {
    DpiScope dpi;
    // Explorer may still be establishing the shell when Windhawk injects it.
    for (int i=0;i<100;++i) {
        DWORD shellPid=0; GetWindowThreadProcessId(GetShellWindow(),&shellPid);
        if (shellPid) { if (shellPid!=GetCurrentProcessId()) return 0; break; }
        if (WaitForSingleObject(g_stop,100)==WAIT_OBJECT_0) return 0;
        if (i==99) return 0;
    }
    const HRESULT co=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if (FAILED(co)) return 0;
    BrokerState state;
    g_broker=&state;
    state.graphics.Init();  // Warm once; no rendering is performed while idle.
    WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc);
    wc.hInstance=g_module; wc.lpfnWndProc=OverlayProc; wc.lpszClassName=kOverlayClass;
    ATOM overlayClass=RegisterClassExW(&wc);
    wc.lpfnWndProc=BrokerProc; wc.lpszClassName=kBrokerClass;
    ATOM brokerClass=RegisterClassExW(&wc);
    HWND control=brokerClass && overlayClass ? CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,
        kBrokerClass,L"",WS_POPUP,0,0,0,0,nullptr,nullptr,g_module,nullptr) : nullptr;
    if (control) {
        Wh_Log(L"BROKER_READY protocol=0.3.1 hwnd=%p closing_device=%d; restart target apps if CLIENT_READY is absent",
            control,state.graphics.composition.Get()!=nullptr);
        ULONGLONG idleDeadline=GetTickCount64()+g.idleSeconds*1000;
        while (WaitForSingleObject(g_stop,0)!=WAIT_OBJECT_0) {
            MSG message{};
            unsigned processed=0;
            while (processed++<64 && PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
                TranslateMessage(&message); DispatchMessageW(&message);
            }
            const DWORD restoreWait=UpdateRestoreTickets();
            const bool live=UpdateAnimations();
            if (!state.animations.empty()) idleDeadline=GetTickCount64()+g.idleSeconds*1000;
            else if (state.graphics.composition && GetTickCount64()>=idleDeadline) state.graphics.Reset();
            if (live) {
                // DWM presents the live-thumbnail updates on composition
                // boundaries. No Sleep(1), busy-spin or timeBeginPeriod loop.
                if (FAILED(DwmFlush())) {
                    for (auto& a:state.animations) if (LiveOperation(a->config.operation))
                        InterlockedExchange(&a->packet->state,Cancelled);
                }
            } else {
                DWORD timeout=restoreWait;
                const ULONGLONG now=GetTickCount64();
                for (auto& a:state.animations) {
                    const ULONGLONG remaining=a->deadline>now ? a->deadline-now : 0;
                    const DWORD wait=a->running ? static_cast<DWORD>(remaining) : 8;
                    timeout=std::min(timeout,wait);
                }
                if (state.animations.empty() && state.graphics.composition) {
                    timeout=std::min(timeout,static_cast<DWORD>(idleDeadline>now ? idleDeadline-now : 0));
                }
                MsgWaitForMultipleObjects(1,&g_stop,FALSE,timeout,QS_ALLINPUT);
            }
        }
        UpdateRestoreTickets(true);
        state.animations.clear();
        DestroyWindow(control);
    }
    g_broker=nullptr;
    if (brokerClass) UnregisterClassW(kBrokerClass,g_module);
    if (overlayClass) UnregisterClassW(kOverlayClass,g_module);
    state.graphics.Reset();
    CoUninitialize();
    return 0;
}

bool ExcludedProcess(std::wstring_view name) {
    std::wstring_view list=g.excluded;
    while (!list.empty()) {
        const auto end=list.find(L';'); auto entry=list.substr(0,end);
        while (!entry.empty() && (entry.front()==L' ' || entry.front()==L'\t')) entry.remove_prefix(1);
        while (!entry.empty() && (entry.back()==L' ' || entry.back()==L'\t')) entry.remove_suffix(1);
        if (!entry.empty() && CompareStringOrdinal(entry.data(),static_cast<int>(entry.size()),
            name.data(),static_cast<int>(name.size()),TRUE)==CSTR_EQUAL) return true;
        if (end==std::wstring_view::npos) break;
        list.remove_prefix(end+1);
    }
    return false;
}
template <typename F> bool Hook(F target,F replacement,F* original) {
    return Wh_SetFunctionHook(reinterpret_cast<void*>(target),reinterpret_cast<void*>(replacement),
                              reinterpret_cast<void**>(original))!=FALSE;
}

HRESULT WINAPI TaskbarSwitchHook(void* self,void* item,BOOL noMinimize) {
    const DWORD error=GetLastError();
    const DWORD inputTime=static_cast<DWORD>(GetMessageTime());
    if (!g_taskbarInputSeen || inputTime!=g_taskbarInputTime) {
        g_taskbarInputSeen=true; g_taskbarInputTime=inputTime;
        unsigned cancelled=0;
        {
            std::lock_guard lock(g_restoresMutex);
            for (auto& ticket:g_restoreTickets) if (AppMotion::ClaimRestoreFallback([&](int desired,int expected) {
                return InterlockedCompareExchange(&ticket->packet->state,desired,expected);
            })) ++cancelled;
        }
        if (cancelled && g.diagnostics) Wh_Log(L"RESTORE_SUPERSEDED newer taskbar input cancelled %u unclaimed request(s)",cancelled);
        if (cancelled) if (HWND broker=Broker()) PostMessageW(broker,kWake,0,0);
    }
    struct Scope {
        HWND previous=g_taskbarRoutedWindow;
        Scope() { ++g_taskbarDepth; g_taskbarRoutedWindow=nullptr; }
        ~Scope() { g_taskbarRoutedWindow=previous; --g_taskbarDepth; }
    } scope;
    const unsigned routes=g_restoreRouteCount;
    const double start=g.diagnostics ? PreciseMs() : 0;
    SetLastError(error);
    const HRESULT result=g_taskbarSwitch(self,item,noMinimize);
    const DWORD nativeError=GetLastError();
    if (g.diagnostics) Wh_Log(L"TASKBAR_DISPATCH duration_ms=%.2f cooperative_restore=%d; this is API time, not click-to-frame latency",
        PreciseMs()-start,g_restoreRouteCount!=routes);
    SetLastError(nativeError); return result;
}

bool InstallTaskbarHook() {
    HMODULE taskbar=GetModuleHandleW(L"taskbar.dll");
    if (!taskbar) {
        Wh_Log(L"TASKBAR_HOOK_UNAVAILABLE taskbar.dll is not loaded; re-enable after the taskbar appears. Other app transitions remain active.");
        return false;
    }
    WH_FIND_SYMBOL symbol{};
    HANDLE search=Wh_FindFirstSymbol(taskbar,nullptr,&symbol);
    if (!search) {
        Wh_Log(L"TASKBAR_HOOK_UNAVAILABLE taskbar symbols could not be loaded; check Windhawk's symbol download log.");
        return false;
    }
    void* address=nullptr;
    do {
        if (symbol.symbol && wcscmp(symbol.symbol,
            L"public: virtual long __cdecl CTaskBand::SwitchTo(struct ITaskItem *,int)")==0) {
            address=symbol.address; break;
        }
    } while (Wh_FindNextSymbol(search,&symbol));
    Wh_FindCloseSymbol(search);
    if (!address) {
        Wh_Log(L"TASKBAR_HOOK_UNAVAILABLE required SwitchTo symbol absent on this Windows build; no offsets are guessed.");
        return false;
    }
    if (!Hook(reinterpret_cast<TaskbarSwitchFn>(address),TaskbarSwitchHook,&g_taskbarSwitch)) {
        Wh_Log(L"TASKBAR_HOOK_UNAVAILABLE SwitchTo hook registration failed."); return false;
    }
    Wh_Log(L"TASKBAR_HOOK_READY minimized-window restores can route to cooperative source clients.");
    return true;
}
}  // namespace

BOOL Wh_ModInit() {
    using VersionFn=void(WINAPI*)(DWORD*,DWORD*,DWORD*);
    const auto version=reinterpret_cast<VersionFn>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlGetNtVersionNumbers"));
    DWORD major=0,minor=0,build=0;
    if (!version) return FALSE;
    version(&major,&minor,&build);
    if (major!=10 || (build&0x0FFFFFFF)<22000) return FALSE;
    g.open=Wh_GetIntSetting(L"animateOpen")!=0; g.close=Wh_GetIntSetting(L"animateClose")!=0;
    g.restore=Wh_GetIntSetting(L"animateRestore")!=0;
    g.restoreMs=AppMotion::Clamp(Wh_GetIntSetting(L"restoreDurationMs"),60,250);
    g.restoreOpacity=AppMotion::Clamp(Wh_GetIntSetting(L"restoreStartOpacityPercent"),0,100);
    g.restoreDispatchMs=AppMotion::Clamp(Wh_GetIntSetting(L"restoreDispatchTimeoutMs"),20,120);
    g_restoreMessage=RegisterWindowMessageW(L"NativeAppMotion.Restore.031.8F23A10C");
    if (!g_restoreMessage) { g.restore=false; Wh_Log(L"TASKBAR_HOOK_UNAVAILABLE restore message registration failed."); }
    g_finishRevealMessage=RegisterWindowMessageW(L"NativeAppMotion.FinishReveal.031.8F23A10C");
    if (!g_finishRevealMessage) { g.open=false; g.restore=false; Wh_Log(L"REVEAL_UNAVAILABLE recovery message registration failed."); }
    g.hides=Wh_GetIntSetting(L"animateTrayHides")!=0; g.battery=Wh_GetIntSetting(L"disableOnBattery")!=0;
    g.respectEffects=Wh_GetIntSetting(L"respectAnimationEffects")!=0;
    g.diagnostics=Wh_GetIntSetting(L"diagnostics")!=0;
    g.openMs=AppMotion::Clamp(Wh_GetIntSetting(L"openDurationMs"),80,400);
    g.closeMs=AppMotion::Clamp(Wh_GetIntSetting(L"closeDurationMs"),60,350);
    g.scale=AppMotion::Clamp(Wh_GetIntSetting(L"scalePercent"),90,100);
    g.concurrent=AppMotion::Clamp(Wh_GetIntSetting(L"maxConcurrent"),1,4);
    g.megapixels=AppMotion::Clamp(Wh_GetIntSetting(L"snapshotMegapixels"),1,8);
    g.handoffMs=AppMotion::Clamp(Wh_GetIntSetting(L"handoffTimeoutMs"),8,80);
    g.idleSeconds=AppMotion::Clamp(Wh_GetIntSetting(L"gpuIdleSeconds"),1,120);
    const auto curve=GetString(L"easing");
    g.curve=curve==L"out" ? AppMotion::Curve::Out : curve==L"linear" ? AppMotion::Curve::Linear : AppMotion::Curve::Smooth;
    if (GetString(L"motionPreset")!=L"custom") {
        static_cast<AppMotion::Tuning&>(g)=AppMotion::Balanced60;
        Wh_Log(L"PRESET balanced60 open=133 close=100 restore=100 opacity=50 scale=97 snapshot_mp=2 handoff=34; individual tuning fields are overridden");
    }
    g.excluded=GetString(L"excludedProcesses");
    DWORD session=0;
    if (!ProcessIdToSessionId(GetCurrentProcessId(),&session) || session==0 || !GetModuleHandleW(L"user32.dll")) return FALSE;
    // Keep IPC at the shell's normal integrity level. An elevated/sandboxed
    // app must not be cloaked while its request is inaccessible to Explorer.
    Handle token;
    alignas(TOKEN_MANDATORY_LABEL) BYTE integrity[512]{};
    DWORD integrityBytes=0;
    if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token.value) ||
        !GetTokenInformation(token.value,TokenIntegrityLevel,integrity,sizeof(integrity),&integrityBytes)) return FALSE;
    auto* label=reinterpret_cast<TOKEN_MANDATORY_LABEL*>(integrity);
    const BYTE subCount=*GetSidSubAuthorityCount(label->Label.Sid);
    if (!subCount) return FALSE;
    const DWORD rid=*GetSidSubAuthority(label->Label.Sid,subCount-1);
    if (rid<SECURITY_MANDATORY_MEDIUM_RID || rid>=SECURITY_MANDATORY_HIGH_RID) return FALSE;
    wchar_t executable[32768]{};
    const DWORD count=GetModuleFileNameW(nullptr,executable,ARRAYSIZE(executable));
    if (!count || count>=ARRAYSIZE(executable)) return FALSE;
    const wchar_t* name=wcsrchr(executable,L'\\'); name=name ? name+1 : executable;
    const bool explorer=_wcsicmp(name,L"explorer.exe")==0;
    if (ExcludedProcess(name) && !explorer) return FALSE;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&Wh_ModInit),&g_module);
    if (!g_module) return FALSE;
    if (g.open || g.close || g.restore) {
        if (!Hook(&ShowWindow,ShowHook,&g_show) || !Hook(&SetWindowPos,PositionHook,&g_position) ||
            !Hook(&DestroyWindow,DestroyHook,&g_destroy) ||
            !Hook(&DwmSetWindowAttribute,AttributeHook,&g_setAttribute) ||
            !Hook(&SetWindowDisplayAffinity,AffinityHook,&g_setAffinity) ||
            !Hook(&CreateWindowExW,CreateWHook,&g_createW) || !Hook(&CreateWindowExA,CreateAHook,&g_createA)) return FALSE;
    }
    if (g.open || g.restore) {
        if (!Hook(&DispatchMessageW,DispatchWHook,&g_dispatchW) || !Hook(&DispatchMessageA,DispatchAHook,&g_dispatchA)) return FALSE;
        if (g.restore && explorer && InstallTaskbarHook()) {
            if (!Hook(&ShowWindowAsync,ShowAsyncHook,&g_showAsync) ||
                !Hook(&SwitchToThisWindow,SwitchWindowHook,&g_switchWindow)) return FALSE;
        }
    }
    Wh_Log(L"CLIENT_INIT protocol=0.3.1 process=%s pid=%lu open=%d close=%d restore=%d; enable diagnostics for transition/fallback events",
        name,GetCurrentProcessId(),g.open,g.close,g.restore);
    return TRUE;
}

void Wh_ModAfterInit() {
    // Resources are created after hook registration succeeds; a failed init
    // cannot leave a background thread running in an unloaded module.
    if (g.open || g.restore) {
        g_timerQueue.store(CreateTimerQueue());
        if (!g_timerQueue.load()) Wh_Log(L"REVEAL_UNAVAILABLE local recovery timer queue could not be created.");
    }
    wchar_t executable[32768]{};
    GetModuleFileNameW(nullptr,executable,ARRAYSIZE(executable));
    const wchar_t* name=wcsrchr(executable,L'\\'); name=name ? name+1 : executable;
    if ((g.open || g.close || g.restore) && _wcsicmp(name,L"explorer.exe")==0) {
        g_stop=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        if (g_stop) g_brokerThread=CreateThread(nullptr,0,BrokerThread,nullptr,0,&g_brokerThreadId);
        if (!g_brokerThread) Wh_Log(L"BROKER_UNAVAILABLE renderer thread could not be started.");
    }
}

void Wh_ModBeforeUninit() {
    g_unloading.store(true,std::memory_order_relaxed);
    HANDLE timers=nullptr;
    { std::lock_guard lock(g_requestsMutex); timers=g_timerQueue.exchange(nullptr); }
    if (timers) DeleteTimerQueueEx(timers,INVALID_HANDLE_VALUE);
    std::vector<std::shared_ptr<Request>> pending;
    { std::lock_guard lock(g_requestsMutex); pending.swap(g_openRequests); }
    for (auto& r:pending) { InterlockedExchange(&r->packet->state,Cancelled); RestoreOpen(*r->packet); }
    pending.clear();
    if (g_stop) SetEvent(g_stop);
    if (g_brokerThread) { WaitForSingleObject(g_brokerThread,INFINITE); CloseHandle(g_brokerThread); g_brokerThread=nullptr; }
    if (g_stop) { CloseHandle(g_stop); g_stop=nullptr; }
    EnumWindows([](HWND hwnd,LPARAM)->BOOL {
        DWORD pid=0; GetWindowThreadProcessId(hwnd,&pid);
        if (pid==GetCurrentProcessId()) {
            RemovePropW(hwnd,kTransitionPreference);
            RemovePropW(hwnd,kClientReady);
            RemovePropW(hwnd,kRestorePending);
            RemovePropW(hwnd,kAffinityPreference);
            RemovePropW(hwnd,kShownOnce);
        }
        return TRUE;
    },0);
}
void Wh_ModUninit() {}
BOOL Wh_ModSettingsChanged(BOOL* reload) { *reload=TRUE; return TRUE; }
#endif  // APP_MOTION_CORE_TEST

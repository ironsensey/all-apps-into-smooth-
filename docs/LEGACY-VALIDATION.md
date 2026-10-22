# Validation record

This is an experimental source release for the standard Windows 11 x64 taskbar.
The available development machine runs Linux, so actual desktop animation and
focus behavior have not been tested on Windows.

## Checks executed

| Check | Result | What it establishes |
| --- | --- | --- |
| Portable policy checks | 348 passed | Click-action mapping, modifier bypass, async request/state decisions, and executable exclusions |
| Hook adapter checks using Win32 API doubles | 52 passed | Scope/reentrancy, native fallback, last-error/return-value preservation, handle cleanup, settings reload, unload bypass, and initialization failures |
| Windows x64 cross-compilation | Passed with GCC 14.2, C++20, warnings treated as errors | Source compiles against real Windows declarations and official Windhawk API headers |
| Windows DLL link | Passed | Windows/Windhawk imports resolve against SDK/import declarations |
| Export inspection | Passed | Windhawk initialization, teardown and settings callback exports are present |
| Official Windhawk settings validator | 0 issues | Embedded settings meet the published validator's format |
| Official Windhawk metadata parser | Passed | Package identifies version 0.1.0 and targets AMD64 only |

The API doubles do not model the Windows compositor, input queues, application
message pumps, taskbar implementation or all foreground-window restrictions.
The cross-build is not a build using Windhawk's bundled compiler and does not
establish real Windows compatibility.

Reference revisions used for ABI/header checks:

- `ramensoftware/windhawk`: `0cfc358d26fcbfcc24097d6b66e84848ea6b8c13`
- `ramensoftware/windhawk-mods`: `3f11069419fad9103a56cf78b28d293fee6c20cf`

## Required Windows checks — not yet run

Compile and enable the source in current Windhawk after the normal Windows 11
taskbar appears. Record the Windows build from `winver`, Windhawk version, display
refresh rate and GPU/driver. Begin with default settings and compare each case
with the mod disabled. Keep other animation/taskbar mods disabled for comparison.

| Case | Expected result |
| --- | --- |
| Ordinary app launch and close | Original Windows/app animation; no replacement overlay or changed close logic |
| Single-window taskbar toggle | Native minimize/restore behavior with defaults |
| Repeated taskbar clicks | Native toggling, or continued visibility with Keep active app open; no mod debounce |
| Multiple windows in one grouped button | Direct native cycling when configured; native chooser in Windows default mode |
| Ctrl, Shift or Alt held | Original modified-click behavior when modifier bypass is enabled |
| A maximized window minimized/restored | Original maximized placement preserved |
| Save dialog, modal dialog or owned window | Native behavior; experimental async path not used |
| Pinned launcher with no running window | Original launch behavior |
| Multiple monitors and virtual desktops | Correct destination, visibility, placement and focus |
| Browser, File Explorer, WinUI app, elevated app | Correct focus and taskbar state, including apps using paths this mod does not intercept |
| Settings changes and disable/re-enable | Clean reload; disabled mod restores native click behavior |
| Experimental minimize enabled alone | Eligible intercepted requests are logged and accepted; no stale visual state during repeated clicks |
| Experimental restore enabled alone | Correct focus/placement after queued restore; disable if this differs undesirably |
| Excluded executable | No experimental async request for that process |
| Explorer restart or sign-in | Check initialization log; re-enable after the taskbar appears if its DLL was not yet loaded |
| Animation effects disabled in Windows | System preference continues to be respected |
| Idle and battery use | Measure against a disabled baseline; no additional periodic mod worker is expected |

Do not infer improved FPS, battery life, reduced latency or a resolved black
flash from compilation or these automated checks. Those claims require desktop
measurements. This package does not promise a universal animation fix or zero bugs.

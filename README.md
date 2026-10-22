# Native Smooth Taskbar

A Windhawk source mod for the **standard Windows 11 x64 taskbar**, designed to
keep Windows' original animations and avoid adding a second animation renderer.

**Experimental source release, not yet tested on a Windows desktop.** It cannot
guarantee jitter-free animation, instant application startup, or a fix for an
existing black flash. Opening and closing are left to Windows and the application;
the mod does not accelerate those animations or alter DWM's frame scheduling.

## Install

1. Install or open the current version of [Windhawk](https://windhawk.net/).
2. Choose **Create a new mod**. Replace the entire template with
   [`native-smooth-taskbar.wh.cpp`](native-smooth-taskbar.wh.cpp).
3. Select **Compile Mod** and enable it. The first load may need to download
   Microsoft's taskbar symbols. Enable it after the desktop/taskbar has appeared.
4. Start with the default settings. Keep **Animation effects** enabled under
   Windows Settings > Accessibility > Visual effects if you want native motion.

If Windhawk logs that `taskbar.dll` is not loaded during Explorer startup,
disable/re-enable the mod once the taskbar is visible. If required symbols cannot
be found, the mod stops initialization. There is no pattern-scanning fallback.

Use the standard taskbar. ARM64, ExplorerPatcher, StartAllBack and animation
replacement mods are outside this release's supported target. Windows 11 build
numbers can change shell internals; compiling does not establish compatibility
with every build.

## Settings

| Setting | Default | Effect |
| --- | --- | --- |
| Enable taskbar customization | On | Master switch |
| Click a grouped app | Cycle windows directly | Uses Explorer's native Ctrl+click action without opening the thumbnail chooser first |
| Click the active app again | Windows default toggle | Optional **Keep active app open** avoids taskbar-click minimization |
| Hold Ctrl, Shift or Alt for native behavior | On | Temporarily bypasses customization and preserves modified-click behavior |
| Experimental nonblocking minimize | Off | Converts eligible task-switching `ShowWindow(SW_MINIMIZE)` calls to `ShowWindowAsync` |
| Experimental nonblocking restore | Off | Converts eligible minimized-window restore calls inside task switching to `ShowWindowAsync(SW_RESTORE)` |
| Apps excluded from experimental async requests | Empty | Exact executable basenames separated by semicolons; case-insensitive |
| Diagnostic logging | Off | Records changed requests/fallbacks when Windhawk logging is also enabled |

Settings changes reload the mod, so no settings mutex runs on the click path.
Exclusions affect the experimental async settings only, not grouping or keep-open
behavior. Use names such as `game.exe;editor.exe`, not paths or wildcards.

**Recommended:** retain defaults first. For repeated clicks that should leave an
app visible, select **Keep active app open**. If you want repeated clicks to
minimize/restore, retain **Windows default toggle**. No clicks are debounced.

The async settings trade synchronous behavior for handing work to the app's own
UI thread. They are not an animation-speed control. Enable **one at a time** and
test focus, maximized windows, and rapid clicking. A hung app still cannot render
or process its request. The app may receive focus later. Other shell paths are
unaffected; diagnostic logs show whether a request actually used this path.

## Resource behavior and safeguards

- Injects into Explorer only, with two shell hooks in the default configuration.
- Adds no rendering loop, screenshots, overlays, polling worker, per-frame work,
  high-resolution timer request, or CPU/GPU priority adjustment.
- Adds no animation queue or animation-completion delay.
- Async hooks are installed only when their settings are enabled.
- Unknown, owned/modal, cloaked, tool and Explorer-owned windows use the original
  API path. Restore requests for non-minimized windows also use the original path,
  preserving maximized-window semantics.
- A failed async submission falls back to the original API. An accepted request
  is not resubmitted. `ShowWindow`'s previous-visibility return value is preserved.
- The mod does not write registry preferences, force-enable accessibility effects,
  disable VSync, change display refresh rate, or hook `dwm.exe`.

These are implementation properties, **not measured latency, battery, RAM or FPS
improvements**. Native Windows animation still uses CPU/GPU resources. The actual
benefit depends on the app, taskbar path, hardware and Windows build.

## Validation

See [VALIDATION.md](VALIDATION.md) for executed checks, their limits and the Windows
test matrix. Portable tests run with:

```sh
./scripts/test.sh
```

A separate cross-build check accepts a MinGW C++ compiler and headers from the
official Windhawk source checkout. It builds a Windows object and links a DLL
against an import library generated from Windhawk's real export declarations:

```sh
MINGW_CXX=/path/to/x86_64-w64-mingw32-g++ \
MINGW_DLLTOOL=/path/to/x86_64-w64-mingw32-dlltool \
WINDHAWK_SOURCE=/path/to/windhawk ./scripts/cross-build.sh
```

That DLL is a validation artifact. Install the source through Windhawk, which
provides its own compiler, engine import library and module registration.

## Remove

Disable/remove this mod in Windhawk. No registry settings need reverting. A
window request accepted immediately before disabling may finish normally.

## Reference and provenance

The supplied *Windows Animations - Fork* reference was inspected for context;
none of its renderer, capture, cloaking or animation code was copied. Because
that reference mod was not installed on the user's PC, it cannot explain their
existing flash.

Explorer symbol names, signatures and click-action constants were checked against
the published Windhawk
[taskbar-left-click-cycle](https://github.com/ramensoftware/windhawk-mods/blob/main/mods/taskbar-left-click-cycle.wh.cpp)
and
[taskbar-no-minimize](https://github.com/ramensoftware/windhawk-mods/blob/main/mods/taskbar-no-minimize.wh.cpp)
sources. This mod has its own implementation and tests. Windows API declarations
and Windhawk API headers are used for compilation; SDK files are not redistributed.

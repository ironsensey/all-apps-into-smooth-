# Native-style App Motion

**Copy [`native-app-motion.wh.cpp`](native-app-motion.wh.cpp) into Windhawk.**

Version 0.3.1 implements **app opening, closing and supported taskbar restore
animations**, recreating a Windows-style scale-and-fade appearance with GPU
composition. It replaces the earlier taskbar experiment.

This update addresses the reported flash/reappearance: normal reveal completion
now presents the final thumbnail, reveals the real source beneath it and waits
for source presentation before releasing the cover and native transition
suppression. Redundant show/layout calls preserve an active reveal, startup
hide/show cycles cannot repeat the custom launch effect on the same window, and
an app cannot re-enable its native animation halfway through a custom reveal.
A local recovery worker handles completion when Explorer cannot change the source.

**Use Motion preset → Balanced 60 Hz (recommended).** This is the new default,
selected for the supplied Ryzen 5600X, RTX 3060 12 GB, 16 GB RAM and 1360×768/60 Hz
display. It applies all timing/resource values together even when Windhawk keeps
individual values saved from an older version. It is a conservative starting
configuration, not a measured hardware optimum. Select Custom settings only to
use the individual tuning fields.

**Experimental, not yet tested on a real Windows desktop.** Cross-compilation and
portable checks pass. Actual appearance, focus, timing, compatibility and resource
use still require Windows testing. This is a custom approximation of Windows'
visual style, not pixel-identical private Windows transition code. It cannot
promise zero stutter, fix every black flash, or make app content render sooner.

## Install

1. Use current Windhawk on **Windows 11 x64 with the standard Explorer shell**.
2. Disable the older **Native Smooth Taskbar** mod and other animation replacements.
3. Choose **Create a new mod** in Windhawk. Replace the whole template with
   [`native-app-motion.wh.cpp`](native-app-motion.wh.cpp), then **Compile Mod**.
4. Enable it while Explorer is running. Restart the apps you want to test so the
   new mod loads in them. Both Explorer's renderer and the source app hooks are
   required.
5. Select **Balanced 60 Hz (recommended)** in **Motion preset**. Keep opening,
   closing and restoration enabled; keep explicit hides off. Leave Windows'
   **Animation effects** enabled under Settings > Accessibility > Visual effects.
6. After upgrading from 0.3.0, restart Windows once so Explorer and all apps load
   the same version. Replace the existing mod rather than enabling another copy.

The optional taskbar restore hook may download one Microsoft taskbar symbol on
first use. Missing symbols leave taskbar routing native; other app effects can
still run. No `dwm.exe` injection is required.
Disable/remove the mod in Windhawk to remove its hooks and restore windows
temporarily concealed for an opening animation. No registry preferences are
written.

## What actually runs

**Opening:** the app continues painting into DWM. An Explorer-hosted live thumbnail
fades/scales in while the real window is temporarily cloaked. Updates follow DWM
composition boundaries, using a high-resolution progress clock. A one-shot
watchdog in the app restores visibility if the renderer fails or disappears.
Completion uses two compositor boundaries: first the full-size opaque thumbnail,
then the real source, before the overlay is removed. These waits run on the
broker or a source recovery worker, not on the app's UI thread. Redundant
show/no-op layout calls do not interrupt the effect. A real move, resize, hide
or minimize still cancels it.
Failures can reveal the real window immediately without an opening animation.

**Closing:** the mod captures an unobscured window once, uploads it once and
submits GPU scale/opacity curves to DirectComposition. Explorer owns the effect,
so it can finish even when the app closes its final window and exits. The app
waits only for a bounded preparation handoff, **not the full closing animation**.
Capture and native API execution time are additional costs. A late/failed
handoff uses the native close.

**Taskbar restore:** for supported minimized windows, Explorer's existing task
switch routine forwards the restore to the app's confirmed UI dispatcher. The
default is a 100 ms ease-out with 50% first-frame visibility, providing earlier
feedback than a completely transparent starting frame. No snapshot or
animation-completion wait is added to the taskbar thread. An atomic deadline
protocol prevents a delayed duplicate restore from unmaximizing a window.
Internal restore APIs within one activation share the accepted request; a newer
input can cancel unclaimed work. New clicks retain a native path while old work
is pending. This is not a measured click-to-frame improvement.

Minimize/maximize, switching to an already visible app, and taskbar selection
policy remain native. A busy app's UI thread and app startup/loading time are
not accelerated. A close/minimize during a reveal cancels that reveal before
the newer operation.

## Customization

| Setting | Default | Purpose |
| --- | --- | --- |
| Animate app opening / closing | Both on | Toggle each transition separately |
| Animate restoration from the taskbar | On | Enable supported minimized-window restores |
| Motion preset | Balanced 60 Hz | Applies timing/resource values together; select Custom to edit them |
| Restore duration / first-frame visibility | 100 ms / 50% | Fast-start ease-out; tune the visual feedback |
| Restore dispatch deadline | 50 ms | Unclaimed work can fall back; claimed requests are not replayed |
| Opening / closing duration | 133 / 100 ms | Curve duration; presentation/setup time is additional |
| Smallest scale | 97% | Subtle Windows-style motion; 100% gives fade only |
| Opening/closing motion curve | Fast start, gentle finish | Restore uses fast-start ease-out independently |
| Maximum simultaneous animations | 2 | Limit active renderer work; extra requests fall back |
| Closing snapshot budget | 2 megapixels | Full resolution at 1360×768; larger windows may be downscaled |
| Closing handoff wait | 34 ms | Limit how long the app waits for renderer readiness |
| Release closing GPU resources after idle | 15 seconds | Drop the device/libraries after inactivity; later cold starts can fall back |
| Animate explicit hides | Off | Optional tray-hide effects |
| Use native animations on battery | On | Battery-first behavior; no effect on an AC-powered desktop |
| Respect Windows animation preferences | On | Honor disabled animation effects |
| Excluded apps | Empty | Exact executable names separated by semicolons |
| Diagnostic logging | Off | Identify custom animations and native fallbacks |

Balanced 60 Hz overrides duration, opacity, dispatch deadline, scale, easing,
concurrency, snapshot budget, handoff, GPU idle and battery fields. Saved custom
values can still appear in those fields but are inactive until Custom is selected.
Animation toggles, Windows animation preference, exclusions and logging remain
independent. At 1360×768, a full-screen BGRA snapshot is about 4 MiB. The pixel
budget limits each image, not all driver-managed memory or overall mod memory.

## Resource design and limits

- GPU device libraries load dynamically **in the Explorer renderer**, not in
  every injected app. The closing device is released after the configured idle
  period.
- Closing curves run in the compositor: no per-frame screenshot or CPU bitmap
  animation loop.
- Opening uses a live GPU thumbnail; CPU updates its geometry/opacity only during
  an active opening. DWM synchronization paces those updates.
- No global timer-resolution request, busy-spin loop, process-priority boost or
  permanent idle animation loop is added.
- Source processes use small one-shot recovery timers. Captured pixels stay in
  short-lived memory mappings; they are not written to disk or logged.

These are implementation properties. **Lower power, CPU/GPU use, RAM use, or
better frame pacing than native Windows have not been measured or established.**

## Coverage and fallback

First shows through `ShowWindow`/`SetWindowPos` and closes through `DestroyWindow`
are intercepted. Nonactivating/background first shows, minimized launches,
elevated/sandboxed/protected processes, foreign-thread operations, owned/modal or
layered windows, offscreen/overlapped close captures, other APIs and direct process
termination can remain native or skip the custom effect. Do not expect universal
coverage of every app. Unsupported thumbnail rendering may also need a per-app
exclusion or disabling opening effects after Windows testing.

Restart apps after enabling. The mod tracks application writes to the transition
preference; preferences set before injection may be unknown. Other shell or
animation replacements are outside this version's supported target.

For taskbar diagnostics, follow [docs/TASKBAR-CHECK.md](docs/TASKBAR-CHECK.md).
`RESTORE_ANIMATION_STARTED` identifies a submitted custom restore. If it is
absent, changing animation duration does not fix an inactive restore path: read
the fallback log first.

See [VALIDATION.md](VALIDATION.md) for executed checks and the pending Windows
test matrix. The older taskbar source is retained only for history; its
documentation is in [docs/LEGACY-TASKBAR.md](docs/LEGACY-TASKBAR.md).

## Development and reference

Run `./scripts/test.sh`. `scripts/cross-build.sh` accepts `MINGW_CXX`,
`MINGW_DLLTOOL` and `WINDHAWK_SOURCE` (the official Windhawk checkout). Generated
DLLs are link-validation artifacts; install the source through Windhawk.

The supplied MIT-licensed *Windows Animations - Fork* by ReDrag informed the
live-thumbnail/captured-image investigation. This mod uses its own renderer
broker, transport, lifecycle and animation implementation. Windows API declarations
and official Windhawk headers are used for compilation and are not redistributed.
The display-affinity fallback follows the documented limitations of
[GetWindowDisplayAffinity](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getwindowdisplayaffinity).

# Native-style App Motion

**Copy [`native-app-motion.wh.cpp`](native-app-motion.wh.cpp) into Windhawk.**

Version 0.2.0 implements **app opening and closing animations**, recreating the
familiar Windows scale-and-fade appearance with GPU composition. It replaces the
earlier taskbar experiment, which did not address app opening/closing.

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
5. Start with defaults. Leave Windows' **Animation effects** enabled under
   Settings > Accessibility > Visual effects.

There are no private-symbol downloads or `dwm.exe` injection requirements.
Disable/remove the mod in Windhawk to remove its hooks and restore windows
temporarily concealed for an opening animation. No registry preferences are
written.

## What actually runs

**Opening:** the app continues painting into DWM. An Explorer-hosted live thumbnail
fades/scales in while the real window is temporarily cloaked. Updates follow DWM
composition boundaries, using a high-resolution progress clock. A one-shot
watchdog in the app restores visibility if the renderer fails or disappears.
Failures can reveal the real window immediately without an opening animation.

**Closing:** the mod captures an unobscured window once, uploads it once and
submits GPU scale/opacity curves to DirectComposition. Explorer owns the effect,
so it can finish even when the app closes its final window and exits. The app
waits only for a bounded preparation handoff, **not the full closing animation**.
Capture and native API execution time are additional costs. A late/failed
handoff uses the native close.

Minimize, maximize, restore and taskbar click policy remain native. A close or
minimize during opening cancels that opening before the native operation; it
does not queue an obsolete animation behind the newer action.

## Customization

| Setting | Default | Purpose |
| --- | --- | --- |
| Animate app opening / closing | Both on | Toggle each transition separately |
| Opening / closing duration | 150 / 120 ms | Tune perceived pace |
| Smallest scale | 96% | Subtle Windows-style motion; 100% gives fade only |
| Motion curve | Smooth | Smooth endpoints, fast-start ease-out, or linear |
| Maximum simultaneous animations | 2 | Limit active renderer work; extra requests fall back |
| Closing snapshot budget | 4 megapixels | Limit each upload; larger windows are downscaled |
| Closing handoff wait | 40 ms | Limit how long the app waits for renderer readiness |
| Release closing GPU resources after idle | 15 seconds | Drop the device/libraries after inactivity; later cold starts can fall back |
| Animate explicit hides | Off | Optional tray-hide effects |
| Use native animations on battery | Off | Optional battery-first behavior |
| Respect Windows animation preferences | On | Honor disabled animation effects |
| Excluded apps | Empty | Exact executable names separated by semicolons |
| Diagnostic logging | Off | Identify custom animations and native fallbacks |

Recommended first test: retain defaults. For less work on battery, enable **Use
native animations on battery**. For tighter texture/CPU-upload limits, reduce the
snapshot budget or concurrency. The pixel budget limits each image, not all
driver-managed memory. Resource limits can trade image sharpness or coverage for
less work.

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

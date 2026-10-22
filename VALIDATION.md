# Native-style App Motion 0.3.1 — validation

## Executed in the Linux development environment

- Windows x64 cross-compilation against MinGW's Windows declarations and official
  Windhawk API headers, with C++20 and warnings treated as errors.
- Windows DLL link and callback export/import inspection.
- Official Windhawk metadata and settings validation.
- `tests/motion_tests.cpp`: 14,405 portable checks covering curve endpoints,
  monotonicity, finite/range behavior, equivalence of live and compositor curves,
  portrait/ultrawide/4K image budgeting, malformed image dimensions/byte counts,
  cancellation/ready races, one-time opening recovery with retry after failure,
  minimized-but-visible restoration, safe affinity-query fallback, supported
  taskbar routing decisions and exclusive source/deadline restore claims. Added
  a compositor model that separates queued changes from presented windows:
  the final thumbnail must present before uncloak, source presentation must
  precede preference restoration and overlay destruction, and completion cannot
  run twice. Geometry checks cover no-op layouts, actual moves/resizes, negative
  monitor coordinates and overflow. The preset preserves 1360×768 capture size.
- The same portable app-motion checks passed with AddressSanitizer and
  UndefinedBehaviorSanitizer enabled; these sanitizers do not cover Windows code.

The numeric count includes sampled curve/geometry cases. It does not represent
14,405 independent app integration tests. Portable atomics do not prove actual
Windows compositor, IPC, driver, focus or timing behavior.

The retained taskbar experiment also passes its 348 policy and 52 mocked-adapter
checks. Those old checks do **not** validate the new app-animation renderer.

Reference SDK: `ramensoftware/windhawk` revision
`0cfc358d26fcbfcc24097d6b66e84848ea6b8c13`. Metadata/settings validator:
`ramensoftware/windhawk-mods` revision
`3f11069419fad9103a56cf78b28d293fee6c20cf`.

## Windows validation not yet performed

The new mod has not been compiled inside Windhawk or run on a Windows desktop
here. No FPS, latency, battery, CPU, GPU or RAM improvement is established. Do not
call it bug-free, universally compatible, or a verified fix for an existing flash.

Record `winver`, Windhawk version, GPU/driver and display refresh rate. Disable the
older taskbar experiment and other animation replacements, then test:

| Scenario | Required observation |
| --- | --- |
| Notepad/browser first launch | Live content fades/scales in; correct focus; no final uncloak flash |
| Repeated ShowWindow / identical SetWindowPos during reveal | No interruption or second appearance |
| Startup hide/show cycle on one HWND | No repeated custom launch effect; original show still runs |
| App changes transition preference during reveal | Native transition stays suppressed until source presentation, then app preference is restored |
| Explorer cannot uncloak source directly | Existing source recovery worker completes handoff; app UI never waits on DwmFlush |
| Upgrade with old saved settings | Balanced 60 Hz applies its effective values; Custom uses saved tuning fields |
| Minimized app restored by its taskbar button | Queue/receive/animation logs appear; correct focus; no full-animation wait on the taskbar UI thread |
| App already visible | Original focus-switch path; no artificial fade or extra restore |
| Taskbar restore hook unavailable | Explicit log; original taskbar activation still works |
| Pending restore followed by another taskbar click | New input takes precedence; no stale overlay or replay |
| App claims restore before deadline | Native deadline fallback cannot issue a second SW_RESTORE |
| Restore maximized window | Original maximized state preserved, including deadline and repeated-click cases |
| Unsupported affinity getter on ordinary window | Live reveal is eligible; close capture requires tracked/queried unrestricted state |
| App with a slow/blank first frame | Visibility recovers; content readiness is not falsely claimed as improved |
| Close last app window/process | Captured close completes in Explorer after source exit |
| Close a non-final window | App's other windows remain responsive |
| Cancel a Save dialog | No false close animation before actual destruction |
| Close/minimize/move during opening | Old opening is cancelled; source cannot remain invisible |
| Renderer stopped/reloaded during opening | Local watchdog restores source visibility and preferences |
| App hooks reloaded during opening | Source recovery precedes timer/context cleanup |
| Failed/late closing handoff | Original close still executes; no late overlay |
| Multiple concurrent requests | Configured active renderer limit respected |
| Overlapped, partly offscreen, protected or owned window | Safe native/visibility fallback |
| Maximized and DPI-scaled windows | Correct geometry, focus and screenshot/thumbnail alignment |
| Multiple monitors and mixed refresh rates | Check frame pacing and handoff on each display |
| Elevated/low-integrity apps | Native path; no delayed invisible window |
| Battery toggle and disabled Windows effects | Configured preference respected |
| Ten-plus minutes idle | No frame loop; closing graphics resources released after idle deadline |
| Repeated launch/close for several hundred cycles | No unbounded handle, mapping, texture or private-memory growth |

Use diagnostics to identify whether a custom effect actually ran. Compare against
the disabled baseline under the same workload. Compilation and portable test
counts do not establish reduced power consumption or smoother frame pacing.

# Check whether taskbar restoration is actually running

Version 0.2 left restoration from the taskbar native. Version 0.3 adds an optional
cooperative path. The custom look is intentionally subtle; appearance alone is
not proof that the mod handled a click or improved frame pacing.

## Update and test

1. Replace the existing mod's source with the current `native-app-motion.wh.cpp`;
   compile and reload it. Disable the old separate taskbar experiment and avoid
   running two versions of App Motion at once.
2. Enable **Animate restoration from the taskbar**. Keep its starting settings:
   duration **110 ms**, first-frame visibility **35%**, scale **96%**.
3. Enable **Diagnostic logging** in the mod settings and enable Windhawk logging
   if it is off. Enabling just the mod setting cannot override disabled platform
   logging.
4. Re-enable after the Windows desktop/taskbar is present if the taskbar DLL was
   unavailable during startup. Permit Windhawk's normal Microsoft symbol download
   if the hook reports that it cannot resolve the taskbar symbol.
5. Restart a normal, non-elevated test app such as Notepad. Minimize it fully,
   then click its taskbar button. This tests restoration, not launching a new
   process or focusing an already visible window.

## Log evidence

| Line | Meaning |
| --- | --- |
| `CLIENT_INIT protocol=0.3` | This process loaded the new version |
| `TASKBAR_HOOK_READY` | Explorer resolved and registered the scoped task-switch hook |
| `BROKER_READY protocol=0.3` | Explorer's renderer control window is available |
| `CLIENT_READY` | The target window's UI thread has passed through the hooked dispatcher |
| `TASKBAR_RESTORE_QUEUED` | An intercepted restore API accepted a cooperative request |
| `RESTORE_RECEIVED dispatch_ms=...` | The app claimed the message; time is queue-to-handler time, not total input/display latency |
| `RESTORE_ANIMATION_STARTED` | The renderer submitted the custom live-thumbnail restore |
| `TASKBAR_DISPATCH ... cooperative_restore=0` | This activation did not route a restore through this path |
| `TASKBAR_NATIVE ...` | Read the reason: client not ready, missing renderer, already visible, pending work, preferences, or hung UI |
| `TASKBAR_HOOK_UNAVAILABLE ...` | Private hook unavailable; activation remains native |
| `RENDERER_NATIVE ...` | Renderer preparation failed, the request was cancelled, or a resource limit was reached |
| `REVEAL_WATCHDOG ...` | Local recovery restored visibility after the renderer did not finish |

If the final animation-start line is absent, a shorter duration does not make the
inactive route work. Report the app name, `winver` version/build, and the relevant
fallback lines. Window titles and image pixels are not included in these logs.

For a temporary visual check only, a longer restore duration (250 ms), 90% scale
and 0% starting visibility make the custom effect easier to distinguish. Return
to 110 ms / 96% / 35% for the intended faster-starting effect. This comparison
does not measure latency or smoothness.

## Limits

Focus switching between already visible windows stays native. Launching an app
still waits for that app to initialize and paint. A hung/busy UI thread cannot
be made responsive by an animation. Other restore APIs, custom taskbars, missing
source injection, denied foreground permission or unsupported window types can
use the native path.

No real Windows desktop validation or power/latency benchmark was performed in
the development environment. The deadline is a scheduled fallback, not a hard
real-time guarantee when the OS, renderer or source app is stalled.

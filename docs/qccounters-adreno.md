# Adreno GPU counters on device (QCCounters)

How `renderdoc/driver/ihv/qc/qc_counters.cpp` samples the Adreno performance-monitor counters,
and the device behaviour each rule in that file exists for. Every statement below was measured on
the hardware listed at the bottom, not inferred from the API.

The 80 published metrics are derived from raw `GL_AMD_performance_monitor` counters with the GGPM
formulas recovered from `libGGPM.so`; see `qc_ggpm_metrics.h`. A metric is published only if every
raw counter its formula reads was measured: a partially measured metric reports no value instead of
a value computed from a zero.

## Sampling sequence

1. A GL context is current (the replay's own context for GL, a private EGL context for Vulkan).
2. `glGenPerfMonitorsAMD` → one monitor for the fetch.
3. One `glSelectPerfMonitorCountersAMD` call per counter, one counter at a time, each followed by
   `glGetError`. A counter the driver rejects is marked refused and the selection is rebuilt without
   it; the caller later gets a pass for that counter on its own.
4. `glEnable(GL_PERFMON_GLOBAL_MODE_QCOM)` (`0x8FA0`) between selection and the first `glBeginPerfMonitorAMD`.
5. Per event: `glBeginPerfMonitorAMD` → replay → `glEndPerfMonitorAMD` → `glFlush` → bounded wait on
   `GL_PERFMON_RESULT_AVAILABLE_AMD` → `GL_PERFMON_RESULT_SIZE_AMD` → `GL_PERFMON_RESULT_AMD`.
6. `glDeletePerfMonitorsAMD`, then `glDisable(GL_PERFMON_GLOBAL_MODE_QCOM)`.

## Device behaviour this handles

**Global mode is mandatory.** The driver allocates the monitor's result buffer only while
`GL_PERFMON_GLOBAL_MODE_QCOM` is enabled. Without it `glGetPerfMonitorCounterDataAMD` dereferences a
null result buffer and the replay process dies inside `libGLESv2_adreno.so`, rather than returning no
data. The enable must be on at `glBeginPerfMonitorAMD`, not just at the query.

**One monitor per process.** After a monitor has been used, a monitor set up later in the same
context reports no result for any window, whatever its selection: `glGenPerfMonitorsAMD` hands the
same name back, deleting it first does not help, and neither `glFinish` nor re-toggling the global
mode before the next monitor changes that. A second *fetch* (which deletes everything and switches
the global mode off at the end) does work. Sampling is therefore done in a single pass by default.

**A dropped selection is silent.** When a block has no room left, or two counters share one hardware
slot, the driver may drop counters — or the whole block — without setting a GL error. Two guards:
per-counter `glGetError` after each select (catches the rejections it does report), and a pass that
reports no result for two samples in a row being abandoned instead of timing out once per event.

**The frame's last events report nothing.** The final event IDs of a frame replay to no GL call, so
the monitor's window is empty and no result is ever published for it. That is expected, costs the
last two samples of the pass, and is why "no result" alone is not treated as a broken selection.

**Hardware block capacity.** On Adreno 610, 26 shader-pipe (SP) counters are needed by the 80
metrics but only 24 fit, so two of them are refused. The single pass selects the counters the most
metrics read first, which keeps the loss to the four metrics those two counters feed. Fetching fewer
metrics at a time always fits.

**Tool layers must be gone.** Snapdragon Profiler's `libq3dtools_esx.so` is injected into every EGL
process while `debug.egl.profiler=1`, and its presence makes the perf-monitor query crash inside the
driver. `Init` refuses to sample when `/proc/self/maps` contains `libq3dtools`
(`RENDERDOC_QC_ALLOW_Q3D=1` overrides that).

A system trace capture already deals with this: before recording it reads `debug.egl.profiler`, and
with root it stops Snapdragon Profiler's service and clears the property, logging the value before and
after. Without root it warns instead, because the counters are refused rather than sampled under the
layer. To do the same by hand:

```
adb shell su -c 'pkill -f sdpservice; pkill -f pluginGPU; pkill -f SnapshotPlayer'
adb shell su -c 'setprop debug.egl.profiler 0; setprop vendor.debug.egl.profiler 0'
```

Order matters - the service resets the property while it runs. On toybox `pkill`, pass no `-9`: its
`-SIGNAL` parsing turns `-9` into a pattern that matches every process and kills the shell running the
command before `setprop` is reached.

## Diagnostics

Device-side switches, read once per process:

| File | Environment variable | Effect |
| --- | --- | --- |
| `/data/local/tmp/qc_debug` | `RENDERDOC_QC_DEBUG=1` | Per-pass and per-event log lines (selected counters, GL errors, result availability). |
| `/data/local/tmp/qc_multipass` | `RENDERDOC_QC_MULTIPASS=1` | Spread the counters over several passes by block budget instead of one pass. Only useful on a device where a second monitor works. |

`logcat` tags every line with `QCCounters:`; the messages name the counters that were refused or lost,
so a user-visible missing metric can be traced to the counter that did not read.

## Measured baseline

Redmi Note 8 (ginkgo, Adreno 610v1, Android 11), `1.rdc` replayed on the device, all 80 metrics
requested at once: 19 counter groups, 70 of 70 needed raw counters resolved, 66 selected, 75 of 80
metrics with a value. The five without:

| Metric | Reason |
| --- | --- |
| `GPU Duration` | GL timer query, not a performance-monitor counter; never part of this set. |
| `Textures / Vertex`, `Textures / Fragment` | Read `SP[33]`, refused: SP has room for 24 of its 26 needed counters. |
| `Interpolation Instructions / Fragment`, `Fragment Interpolation Instructions / Second` | Read `SP[42]`, refused for the same reason. |

Requesting only the metrics that need the refused counters measures them normally, because the
selection then fits.

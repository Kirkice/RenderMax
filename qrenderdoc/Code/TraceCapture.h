/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2026 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once

#include <stdint.h>

#include <functional>

#include <QString>
#include <QStringList>

#include "renderdoc_replay.h"

class QObject;

// ---------------------------------------------------------------------------------------------
// P1 — on-device system trace capture.
//
// This is the capture half of the trace-correlation feature. The importer in SystemTrace.h reads
// a Chrome Trace Event Format file; this driver produces one by driving the device itself:
//
//   1. probe `perfetto` on the device (every Android 10+ device ships it as the system tracer),
//   2. push a Perfetto textproto config,
//   3. record to a protobuf trace on the device,
//   4. pull that trace back,
//   5. convert it to Chrome Trace Event Format with Perfetto's own `traceconv`,
//   6. return the path of the importable JSON.
//
// It reaches the device only through IDeviceProtocolController::ExecCommand / PushFile /
// PullFile, so it works for any protocol that implements them (currently `adb`) and never links
// against an engine or vendor binary. [观测] Neither `perfetto` nor `traceconv` is bundled or
// redistributed by RenderDoc: the device binary is the platform's own, and `traceconv` is located
// on the user's machine (the Snapdragon Profiler distribution ships `traceconv-win-amd64.exe`, and
// a standalone build may be on PATH).
//
// Limits that are real and therefore stated rather than worked around:
//   * A device protocol serialises commands onto one command thread (AndroidController::Invoke
//     spins until the queued command finishes). A second ExecCommand therefore CANNOT interrupt a
//     recording that is already running, so a capture is always bounded by the config's
//     `duration_ms` instead of being stoppable on demand. Cancelling only stops the *pipeline*
//     between steps.
//   * The exact `traceconv` sub-command spelling has varied between Perfetto releases. The probe
//     and conversion results are reported verbatim in TraceCaptureResult::log so a mismatch is
//     visible as evidence instead of being smoothed over.
// ---------------------------------------------------------------------------------------------

// How much of the GPU's own ftrace surface to record. This is a size/coverage trade-off, not a
// capability one: every tracepoint below is requested only when the probe found it present and
// enable-able, so asking for more than a device can give simply yields fewer lanes.
//
// Only the command and fence-wait events scale with what the application is doing: they fire once
// per submission and once per fence, so a submission-heavy frame produces thousands of them, each a
// long line of counters that has to be recorded on the device, converted and parsed here. The
// metrics are sampled at whatever rate the device chooses and cost about the same either way. So the
// default stops at the metrics that answer "how busy was the GPU", and the per-command lanes are
// opt-in. The difference is not always large - a three second capture of a lightly loaded
// application measured 93.9 MB of systrace text without the command events and 96.2 MB with them,
// because on that device the scheduler and trace marker lines dominate - but the ceiling is set by
// the application's submission rate, which this cannot know in advance.
enum class GpuTraceDetail
{
  // No GPU events at all: the smallest trace, schedule and CPU lanes only.
  Off = 0,
  // Frequency, utilisation, temperature, thermal frequency cap and throttling state. These are
  // sampled or event-driven at a rate the device chooses, so they add tens of events per second.
  Metrics = 1,
  // Metrics plus the per-cmdbatch and per-fence intervals, which become the GPU Commands and GPU
  // Waits lanes. Opt-in, because they are the only part whose size follows the workload.
  Commands = 2,
};

struct TraceCaptureOptions
{
  // Perfetto textproto config. When empty, DefaultPerfettoConfig() is used.
  QString configText;
  // Recording length and ring buffer size baked into the default config. Ignored when
  // `configText` is supplied, because then the config owns those values.
  int durationMs = 5000;
  // Which GPU tracepoints to ask for. The probe decides what is actually available; this decides
  // what is worth the space among what is available.
  GpuTraceDetail gpuDetail = GpuTraceDetail::Metrics;
  // A ring buffer rather than a filling one, so a long recording keeps its tail instead of stopping
  // at the ceiling. The default leaves room for the scheduler, interrupt, clock and atrace streams
  // the config enables on a busier device.
  uint64_t bufferSizeKb = 32768;
  // Explicit path to `traceconv`. When empty, discovery is attempted (Snapdragon Profiler install
  // directory, then PATH).
  QString traceconvPath;
  // Directory of the Snapdragon Profiler installation, used only to look for its bundled
  // `traceconv`. Empty disables that discovery step.
  QString snapdragonProfilerPath;
};

// What this device can actually report about the GPU, as measured by probes rather than assumed.
// Every field is the result of a command the capture ran, so an unavailable lane can be explained
// with the reason it is unavailable instead of being silently absent.
//
// The three tiers that matter, in order of how much a device has to allow:
//   * sysfs: `/sys/class/kgsl/kgsl-3d0/{gpuclk,gpu_busy_percentage,devfreq}` is readable by the
//     `shell` user on some builds and refused on others.
//   * tracepoints: `traced_probes` enables the KGSL and thermal events, and its `enable` files are
//     root-owned mode 0644 on a stock build, so it cannot write them without help.
//   * root: `su` grants both the reads and the chmod. It does NOT grant per-stage GPU counters: those
//     need `/sys/class/kgsl/kgsl-3d0/perfcounter`, which only exists in kernels built with
//     CONFIG_QCOM_KGSL_PERFCOUNTER, and it is absent on most production devices.
struct GpuTraceCapabilities
{
  // `su -c id` reported uid=0. When false, every probe below is expected to be refused.
  bool rootAvailable = false;
  // `/sys/class/kgsl/kgsl-3d0/gpuclk` and `gpu_busy_percentage` were readable.
  bool sysfsReadable = false;
  // `/sys/class/kgsl/kgsl-3d0/perfcounter` exists. This is the node SDP reads its per-stage GPU
  // counters from (`% Shader ALU Capacity Utilized`, `% Texture Fetch Stall`, ...), so its absence is
  // exactly why those metrics cannot be produced here.
  bool perfcounterNode = false;
  // ftrace events that are both present in the kernel and enabled for `traced_probes`. Only these go
  // into the recording config: an event that is requested and cannot be enabled records nothing and
  // reports no error, which looks identical to a device that has no GPU.
  QStringList eventNames;
  // The detail level this probe was run for. Kept in the result so a trace without command lanes can
  // be read back as either "they were not asked for" or "the device refused them".
  GpuTraceDetail requestedDetail = GpuTraceDetail::Metrics;
  // Raw probe values, kept verbatim for the capture log.
  QString gpuClock;
  QString gpuBusyPercent;
  QString gpuMaxClock;
};

struct TraceCaptureResult
{
  bool success = false;
  // The pulled Perfetto protobuf. Retained even when conversion fails, so the recording is not
  // lost and the user can convert it later.
  QString localTracePath;
  // The importable timeline: `systrace` text when traceconv supports that sub-command, otherwise
  // Chrome Trace Event Format JSON. ImportSystemTraceFile() accepts either.
  QString localTimelinePath;
  // The persistent local copy of every command and result. This remains available after the UI is
  // closed, so failed captures can be diagnosed without relying on an in-memory log widget.
  QString localLogPath;
  // The path the trace had while on the device, kept for diagnostics.
  QString deviceTracePath;
  QString traceconvPath;
  // Non-empty only on failure, and always names the step that failed plus the tool's own output.
  QString error;
  // What the device reported about its GPU. Retained so the lanes that are present, and the ones
  // that are not, can be traced back to a specific probe result.
  GpuTraceCapabilities gpu;
  // Things the capture did not fail on but that make the resulting timeline incomplete, e.g. a
  // device that will not report its GPU clock. Shown next to the trace so the absence of a lane is
  // explained rather than left for the user to guess at.
  QStringList warnings;
  // Every command issued and every tool result, in order. The UI shows this so a capture failure
  // is diagnosable from what actually ran rather than from a generic message.
  QStringList log;
};

// A default Perfetto config covering the counters this feature correlates against: scheduler
// switches, per-CPU frequency and idle residency, the atrace `gfx`/`sched` categories, and Android
// system memory/CPU statistics. Every event name here is part of the standard Android ftrace
// surface, so the config does not assume an Adreno- or vendor-specific probe exists.
//
// `extraFtraceEvents` adds vendor-shaped events, in `group/event` form. RunTraceCapture passes the
// KGSL and thermal events that its probe found present and enabled, so the GPU lanes exist exactly
// when the device can supply them. An empty list is a valid, complete config for a device without
// those tracepoints.
QString DefaultPerfettoConfig(int durationMs, uint64_t bufferSizeKb,
                              const QStringList &extraFtraceEvents = QStringList());

// Runs the capture pipeline. Owns no state and must be called off the UI thread, because every
// device command blocks until it completes. Never throws: every failure is returned in `error`
// with the failing step recorded in `log`.
TraceCaptureResult RunTraceCapture(IDeviceProtocolController *protocol, const QString &url,
                                   const TraceCaptureOptions &opts);

// Convenience wrapper: runs RunTraceCapture() on a worker thread and delivers the result back on
// `owner`'s thread. `done` is invoked exactly once. The returned thread self-deletes.
class TraceCaptureJob
{
public:
  typedef std::function<void(const TraceCaptureResult &)> Callback;

  static void Start(QObject *owner, IDeviceProtocolController *protocol, const QString &url,
                    const TraceCaptureOptions &opts, Callback done);
};

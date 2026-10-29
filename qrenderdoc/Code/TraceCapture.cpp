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

#include "TraceCapture.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QUuid>

#include "Code/QRDUtils.h"

namespace
{
// The device directory used for the config and the protobuf. /data/local/tmp is the one path adb
// shell can always write without root, so it is used for both files and removed afterwards.
const char *DeviceTempRoot = "/data/local/tmp";

QString ShellQuote(const QString &path)
{
  return QStringLiteral("'") + path + QStringLiteral("'");
}

void Log(TraceCaptureResult &result, const QString &line)
{
  result.log.append(line);
}

// Reports a failure with the step that produced it. Every failure path goes through here so the
// message always names the step, and the caller never has to reconstruct which command broke.
void Fail(TraceCaptureResult &result, const QString &step, const QString &detail)
{
  if(result.error.isEmpty())
    result.error = detail.isEmpty() ? step : (step + QStringLiteral(": ") + detail);
}

// Trims tool output for display but keeps a bounded amount of it. Command output is evidence, so
// it is included rather than replaced with a summary.
QString Summarise(const QString &text, int maxChars = 4000)
{
  QString trimmed = text.trimmed();
  if(trimmed.size() > maxChars)
    trimmed = trimmed.left(maxChars) + QStringLiteral("\n[... truncated]");
  return trimmed;
}

// Turns an rdcstr into a QString for display. rdcstr is UTF-8 on every platform.
QString ToQStringDisplay(const rdcstr &s)
{
  return QString::fromUtf8(s.c_str(), (int)s.size());
}

// Locates `traceconv`. Returns an empty string when it cannot be found, and never guesses a
// directory: a wrong guess would fail later with a confusing error.
QString FindTraceconv(const TraceCaptureOptions &opts, TraceCaptureResult &result)
{
  if(!opts.traceconvPath.isEmpty())
  {
    if(QFileInfo::exists(opts.traceconvPath))
    {
      Log(result, QStringLiteral("traceconv: using configured path %1").arg(opts.traceconvPath));
      return opts.traceconvPath;
    }

    Log(result,
        QStringLiteral("traceconv: configured path %1 does not exist, falling back to discovery")
            .arg(opts.traceconvPath));
  }

  // [observed] Snapdragon Profiler 2025 installs Perfetto's converter below
  // <install>/plugins/data. Older releases place it beside the executable or under Resources.
  // Probe only known layouts so discovery never turns an arbitrary directory into an executable.
  if(!opts.snapdragonProfilerPath.isEmpty())
  {
    const QDir sdpDir(opts.snapdragonProfilerPath);
    const QStringList candidates = {
        QStringLiteral("traceconv-win-amd64.exe"),
        QStringLiteral("traceconv.exe"),
        QStringLiteral("traceconv"),
        QStringLiteral("plugins/data/traceconv-win-amd64.exe"),
        QStringLiteral("plugins/data/traceconv.exe"),
        QStringLiteral("Plugins/data/traceconv-win-amd64.exe"),
        QStringLiteral("Plugins/data/traceconv.exe"),
        QStringLiteral("resources/traceconv-win-amd64.exe"),
        QStringLiteral("Resources/traceconv-win-amd64.exe"),
    };

    for(const QString &candidate : candidates)
    {
      const QString path = sdpDir.absoluteFilePath(candidate);
      if(QFileInfo::exists(path))
      {
        Log(result, QStringLiteral("traceconv: found next to Snapdragon Profiler at %1").arg(path));
        return path;
      }
    }

    Log(result, QStringLiteral("traceconv: no bundled converter found under %1")
                    .arg(opts.snapdragonProfilerPath));
  }

  // A standalone traceconv can be installed beside qrenderdoc. This is deliberately discovery
  // rather than redistribution: RenderMax does not ship the executable, but a local deployment can
  // place Perfetto's official binary next to the application without modifying the machine PATH.
  const QDir applicationDir(QCoreApplication::applicationDirPath());
#ifdef _WIN32
  const QStringList applicationCandidates = {
      QStringLiteral("traceconv.exe"),
      QStringLiteral("traceconv-win-amd64.exe"),
  };
#else
  const QStringList applicationCandidates = {QStringLiteral("traceconv")};
#endif
  for(const QString &candidate : applicationCandidates)
  {
    const QString path = applicationDir.absoluteFilePath(candidate);
    if(QFileInfo::exists(path))
    {
      Log(result, QStringLiteral("traceconv: found beside RenderMax at %1").arg(path));
      return path;
    }
  }

  const QString onPath = QStandardPaths::findExecutable(QStringLiteral("traceconv"));
  if(!onPath.isEmpty())
  {
    Log(result, QStringLiteral("traceconv: found on PATH at %1").arg(onPath));
    return onPath;
  }

#ifdef _WIN32
  // A normal Snapdragon Profiler installation does not put traceconv on PATH. Discover its known
  // default installation location as a final convenience, so System Trace works without requiring
  // a separate external-tool configuration entry.
  const QStringList programFiles = {
      QString::fromLocal8Bit(qgetenv("ProgramW6432")),
      QString::fromLocal8Bit(qgetenv("ProgramFiles")),
      QString::fromLocal8Bit(qgetenv("ProgramFiles(x86)")),
  };
  const QStringList candidates = {
      QStringLiteral("plugins/data/traceconv-win-amd64.exe"),
      QStringLiteral("plugins/data/traceconv.exe"),
  };

  for(const QString &root : programFiles)
  {
    if(root.isEmpty())
      continue;

    const QDir installDir(QDir(root).absoluteFilePath(QStringLiteral("Qualcomm/Snapdragon Profiler")));
    for(const QString &candidate : candidates)
    {
      const QString path = installDir.absoluteFilePath(candidate);
      if(QFileInfo::exists(path))
      {
        Log(result, QStringLiteral("traceconv: found in default Snapdragon Profiler installation at %1")
                        .arg(path));
        return path;
      }
    }
  }
#endif

  Log(result, QStringLiteral("traceconv: not found beside RenderMax, on PATH, or in a default "
                             "Snapdragon Profiler installation"));
  return QString();
}

// Runs a local tool to completion and returns its combined output. `ok` reports whether it started
// and exited normally with code 0.
QString RunLocalTool(const QString &exe, const QStringList &args, int timeoutMs, bool &ok)
{
  ok = false;

  QProcess process;
  process.setProgram(exe);
  process.setArguments(args);
  process.start();

  if(!process.waitForStarted(10000))
  {
    return QStringLiteral("failed to start %1: %2").arg(exe, process.errorString());
  }

  if(!process.waitForFinished(timeoutMs))
  {
    process.kill();
    process.waitForFinished(5000);
    return QStringLiteral("%1 timed out after %2 ms").arg(exe).arg(timeoutMs);
  }

  const QString stdOut = QString::fromUtf8(process.readAllStandardOutput());
  const QString stdErr = QString::fromUtf8(process.readAllStandardError());

  QString combined;
  if(!stdOut.trimmed().isEmpty())
    combined += stdOut;
  if(!stdErr.trimmed().isEmpty())
  {
    if(!combined.isEmpty())
      combined += QStringLiteral("\n");
    combined += stdErr;
  }

  if(process.exitStatus() != QProcess::NormalExit)
  {
    combined += QStringLiteral("\n%1 crashed").arg(exe);
    return combined;
  }

  if(process.exitCode() != 0)
  {
    combined += QStringLiteral("\n%1 exited with code %2").arg(exe).arg(process.exitCode());
    return combined;
  }

  ok = true;
  return combined;
}

// Store diagnostics independently of the viewer: its in-memory Capture Log disappears when the
// application closes, but a device-side error usually needs to be inspected after the failure.
void PersistCaptureLog(TraceCaptureResult &result)
{
  const QString directory =
      QDir(QDir::tempPath()).absoluteFilePath(QStringLiteral("RenderMax/trace-capture-logs"));
  if(!QDir().mkpath(directory))
    return;

  QString suffix = QUuid::createUuid().toString();
  suffix.remove(QLatin1Char('{')).remove(QLatin1Char('}')).remove(QLatin1Char('-'));

  const QString path = QDir(directory).absoluteFilePath(
      QStringLiteral("trace-capture-%1-%2.log")
          .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")))
          .arg(suffix.left(8)));

  QFile file(path);
  if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    return;

  QString text = result.log.join(QStringLiteral("\n"));
  if(!result.error.isEmpty())
    text += QStringLiteral("\n\nFAILED: %1").arg(result.error);
  if(!result.localTracePath.isEmpty())
    text += QStringLiteral("\n\nPerfetto trace: %1").arg(result.localTracePath);
  if(!result.localTimelinePath.isEmpty())
    text += QStringLiteral("\nImportable timeline: %1").arg(result.localTimelinePath);

  if(file.write(text.toUtf8()) == text.toUtf8().size())
  {
    result.localLogPath = path;
    Log(result, QStringLiteral("persistent capture log: %1").arg(path));
  }
}

}    // anonymous namespace

QString DefaultPerfettoConfig(int durationMs, uint64_t bufferSizeKb, bool includeGpuClock)
{
  // Every data source and event named here is part of the standard Android Perfetto surface, so
  // the config does not assume a vendor-specific probe or a rooted device. The events chosen are the
  // ones that explain GPU-bound frames: what ran, on which CPU and at what frequency, what took the
  // CPU away from it (interrupts, softirqs, clock gating), and what a thread was waiting for when it
  // was not running. `atrace_apps: "*"` is what makes the atrace categories apply to the app under
  // capture rather than only to the system processes Perfetto tags by default.
  //
  // The interrupt events are requested by name rather than through the `irq` atrace category: the
  // category also turns on the IPI, tasklet and softirq-raise probes, which are point events the
  // timeline has no lane for, and they only add noise to the warning about undrawn event types.
  // `binder_driver` is left out for the same reason until binder transactions have a lane.
  // Every ftrace event requested here is emitted on stock Android 12; `workqueue`'s probes are
  // compiled out of some vendor kernels, in which case the timeline simply has no workqueue spans.
  //
  // NOTE: this is textproto, selected with `perfetto --txt`.
  return QStringLiteral(
             "duration_ms: %1\n"
             "flush_period_ms: 1000\n"
             "buffers: {\n"
             "    size_kb: %2\n"
             "    fill_policy: RING_BUFFER\n"
             "}\n"
             "data_sources: {\n"
             "    config {\n"
             "        name: \"linux.ftrace\"\n"
             "        ftrace_config {\n"
             "            ftrace_events: \"sched/sched_switch\"\n"
             "            ftrace_events: \"sched/sched_wakeup\"\n"
             "            ftrace_events: \"sched/sched_waking\"\n"
             "            ftrace_events: \"sched/sched_blocked_reason\"\n"
             "            ftrace_events: \"sched/sched_process_exit\"\n"
             "            ftrace_events: \"sched/task_newtask\"\n"
             "            ftrace_events: \"sched/task_rename\"\n"
             "            ftrace_events: \"power/cpu_frequency\"\n"
             "%3"
             "            ftrace_events: \"power/cpu_idle\"\n"
             "            ftrace_events: \"power/suspend_resume\"\n"
             "            ftrace_events: \"clk/clk_set_rate\"\n"
             "            ftrace_events: \"clk/clk_enable\"\n"
             "            ftrace_events: \"clk/clk_disable\"\n"
             "            ftrace_events: \"irq/irq_handler_entry\"\n"
             "            ftrace_events: \"irq/irq_handler_exit\"\n"
             "            ftrace_events: \"irq/softirq_entry\"\n"
             "            ftrace_events: \"irq/softirq_exit\"\n"
             "            ftrace_events: \"workqueue/workqueue_execute_start\"\n"
             "            ftrace_events: \"workqueue/workqueue_execute_end\"\n"
             "            ftrace_events: \"ftrace/print\"\n"
             "            atrace_categories: \"gfx\"\n"
             "            atrace_categories: \"view\"\n"
             "            atrace_categories: \"sched\"\n"
             "            atrace_categories: \"freq\"\n"
             "            atrace_categories: \"idle\"\n"
             "            atrace_categories: \"am\"\n"
             "            atrace_categories: \"wm\"\n"
             "            atrace_categories: \"input\"\n"
             "            atrace_categories: \"res\"\n"
             "            atrace_categories: \"sync\"\n"
             "            atrace_categories: \"hal\"\n"
             // No "workq": on a non-rooted device traced_probes rejects the whole atrace session
             // with 'category "workq" requires root privileges' and then enables no tags at all,
             // so every app run silently loses tracing_mark_write - no userspace lanes and no frame
             // boundaries. The `workqueue/workqueue_execute_*` ftrace events requested above are what
             // the importer actually reads, and those do not need the atrace category.
             "            atrace_categories: \"power\"\n"
             "            atrace_apps: \"*\"\n"
             "        }\n"
             "    }\n"
             "}\n"
             "data_sources: {\n"
             "    config {\n"
             "        name: \"linux.sys_stats\"\n"
             "        sys_stats_config {\n"
             "            meminfo_period_ms: 1000\n"
             "            meminfo_counters: MEMINFO_MEM_TOTAL\n"
             "            meminfo_counters: MEMINFO_MEM_FREE\n"
             "            meminfo_counters: MEMINFO_MEM_AVAILABLE\n"
             "            meminfo_counters: MEMINFO_CACHED\n"
             "            vmstat_period_ms: 1000\n"
             "            vmstat_counters: VMSTAT_PGFAULT\n"
             "            vmstat_counters: VMSTAT_PGMAJFAULT\n"
             "            stat_period_ms: 1000\n"
             "            stat_counters: STAT_CPU_TIMES\n"
             "        }\n"
             "    }\n"
             "}\n")
      .arg(durationMs)
      .arg(bufferSizeKb)
      .arg(includeGpuClock
               ? QStringLiteral("            ftrace_events: \"kgsl/kgsl_pwrlevel\"\n")
               : QString());
}

TraceCaptureResult RunTraceCapture(IDeviceProtocolController *protocol, const QString &url,
                                   const TraceCaptureOptions &opts)
{
  TraceCaptureResult result;

  if(protocol == NULL)
  {
    Fail(result, QStringLiteral("capture"),
         QStringLiteral("no device protocol is available for this host"));
    return result;
  }

  // Every command addresses the device through exactly the URL the caller selected it by, so the
  // device identity is never re-derived from a friendly name or an index.
  const rdcstr urlStr = url.toUtf8().data();

  Log(result, QStringLiteral("device: %1 / protocol %2")
                  .arg(url, ToQStringDisplay(protocol->GetProtocolName())));

  // Filled in by the GPU clock probe below; the default config is built from it.
  bool includeGpuClock = false;

  // -----------------------------------------------------------------------------------------------
  // Step 1: probe perfetto. A device without it cannot be traced by this path at all, and saying so
  // up front is far more useful than a failure part-way through the pipeline.
  // -----------------------------------------------------------------------------------------------
  {
    rdcstr out;
    const ResultDetails probe = protocol->ExecCommand(
        urlStr, "command -v perfetto || ls -l /system/bin/perfetto", &out);

    if(!probe.OK())
    {
      Log(result, QStringLiteral("perfetto probe failed: %1").arg(ToQStringDisplay(probe.Message())));
      Fail(result, QStringLiteral("probe perfetto"),
           QStringLiteral("this device does not expose `perfetto`, which Android 10+ provides as "
                          "the system tracer. Nothing was recorded.\n%1")
               .arg(ToQStringDisplay(probe.Message())));
      return result;
    }

    Log(result, QStringLiteral("perfetto: %1").arg(Summarise(ToQStringDisplay(out), 200)));
  }

  // -----------------------------------------------------------------------------------------------
  // Step 1b: probe whether this device will report a GPU clock. This decides whether
  // `kgsl/kgsl_pwrlevel` goes into the config, so it has to run before the config is written.
  //
  // Two independent gates are checked, because either one alone is misleading:
  //   * the sysfs node `gpuclk` is what any userland tool reads. If the shell user cannot read it,
  //     the GPU clock is not published to userland at all.
  //   * `traced_probes` may still be refused even when the node is readable, because the KGSL
  //     tracepoint's format file carries the `debugfs_tracing_debug` label. That failure is silent
  //     from Perfetto's point of view - it cannot decode the event so it records nothing - which is
  //     why the recording only asks for the event when the probe succeeded.
  // -----------------------------------------------------------------------------------------------
  {
    rdcstr gpuClkOut;
    const ResultDetails gpuClk = protocol->ExecCommand(
        urlStr, QStringLiteral("cat /sys/class/kgsl/kgsl-3d0/gpuclk"), &gpuClkOut);

    // A refused `cat` exits non-zero, and the protocol reports the reason on the failed result
    // rather than on stdout, so both are consulted: the value from stdout, the reason from the
    // error. Either one saying the node is not readable is enough.
    const QString gpuClkText = ToQStringDisplay(gpuClkOut).trimmed();
    const QString gpuClkError = gpuClk.OK() ? QString() : ToQStringDisplay(gpuClk.Message());
    const QString gpuClkReason = gpuClkText.isEmpty() ? gpuClkError : gpuClkText;

    const bool gpuClkReadable =
        gpuClk.OK() && !gpuClkText.isEmpty() &&
        !gpuClkReason.contains(QStringLiteral("Permission denied")) &&
        !gpuClkReason.contains(QStringLiteral("No such file"));

    if(gpuClkReadable)
    {
      includeGpuClock = true;
      Log(result, QStringLiteral("GPU clock: %1 Hz (kgsl/kgsl_pwrlevel requested)")
                      .arg(gpuClkText));
    }
    else
    {
      Log(result, QStringLiteral("GPU clock probe failed: %1").arg(Summarise(gpuClkReason, 200)));

      result.warnings << QStringLiteral(
          "No GPU clock lane: this device will not report the GPU frequency. "
          "/sys/class/kgsl/kgsl-3d0/gpuclk was refused (%1), and the KGSL tracepoint that would "
          "carry it is labelled debugfs_tracing_debug, which an Enforcing SELinux build does not "
          "let traced_probes read. CPU frequency, idle residency, scheduling and atrace slices "
          "are unaffected.")
                             .arg(Summarise(gpuClkReason, 160));
    }
  }

  // -----------------------------------------------------------------------------------------------
  // Step 2: stage the config under /data/local/tmp. A per-run directory keeps concurrent captures
  // separate. The actual trace is written to Perfetto's SELinux-approved output directory below.
  // -----------------------------------------------------------------------------------------------
  // QUuid::WithoutBraces is newer than the Qt versions this project still builds against, so the
  // braces and separators are stripped from the string form instead.
  QString runSuffix = QUuid::createUuid().toString();
  runSuffix.remove(QLatin1Char('{')).remove(QLatin1Char('}')).remove(QLatin1Char('-'));

  const QString runId = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")) +
                        QStringLiteral("-") + runSuffix.left(8);

  const QString deviceDir =
      QStringLiteral("%1/rdoc_trace_%2").arg(QLatin1String(DeviceTempRoot), runId);
  const QString deviceConfig = deviceDir + QStringLiteral("/config.txtproto");
  // Pixel 3's perfetto_cmd can produce a trace only inside this platform-managed directory. The
  // Perfetto process can write it and ADB can pull it, while the shell user cannot create it there.
  const QString deviceTrace =
      QStringLiteral("/data/misc/perfetto-traces/rendermax-%1.perfetto-trace").arg(runId);

  result.deviceTracePath = deviceTrace;

  {
    rdcstr out;
    const ResultDetails mk = protocol->ExecCommand(
        urlStr, QStringLiteral("mkdir -p %1").arg(ShellQuote(deviceDir)), &out);

    if(!mk.OK())
    {
      Log(result, QStringLiteral("mkdir failed: %1").arg(ToQStringDisplay(mk.Message())));
      Fail(result, QStringLiteral("create device staging directory"),
           ToQStringDisplay(mk.Message()));
      return result;
    }

    Log(result, QStringLiteral("staging directory: %1").arg(deviceDir));
  }

  // The local output directory is deliberately NOT a QTemporaryDir: the converted JSON is the
  // artefact the user asked for, so it must survive this function. The path is reported back and
  // logged, which makes it findable without the caller having to remember it.
  const QString localDir = QDir(QDir::tempPath())
                               .absoluteFilePath(QStringLiteral("rdoc-trace-%1").arg(runId));
  if(!QDir().mkpath(localDir))
  {
    Fail(result, QStringLiteral("create local output directory"),
         QStringLiteral("could not create %1").arg(localDir));
    return result;
  }

  const QString localConfig = QDir(localDir).absoluteFilePath(QStringLiteral("config.txtproto"));
  const QString localTrace = QDir(localDir).absoluteFilePath(QStringLiteral("capture.perfetto-trace"));

  result.localTracePath = localTrace;
  Log(result, QStringLiteral("local output directory: %1").arg(localDir));

  // -----------------------------------------------------------------------------------------------
  // Step 3: write and push the config. The config text is caller-supplied when present, so a user
  // who knows which counters they want is not limited to the default surface.
  // -----------------------------------------------------------------------------------------------
  {
    const QString configText =
        opts.configText.isEmpty()
            ? DefaultPerfettoConfig(opts.durationMs, opts.bufferSizeKb, includeGpuClock)
            : opts.configText;

    QFile configFile(localConfig);
    if(!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
      Fail(result, QStringLiteral("write local config"),
           QStringLiteral("could not write %1: %2").arg(localConfig, configFile.errorString()));
      return result;
    }

    const QByteArray utf8 = configText.toUtf8();
    if(configFile.write(utf8) != utf8.size())
    {
      Fail(result, QStringLiteral("write local config"),
           QStringLiteral("short write to %1").arg(localConfig));
      return result;
    }
    configFile.close();

    const ResultDetails push = protocol->PushFile(urlStr, localConfig.toUtf8().data(),
                                                  deviceConfig.toUtf8().data());
    if(!push.OK())
    {
      Log(result, QStringLiteral("push config failed: %1").arg(ToQStringDisplay(push.Message())));
      Fail(result, QStringLiteral("push perfetto config"), ToQStringDisplay(push.Message()));
      return result;
    }

    Log(result, QStringLiteral("pushed %1 byte config to %2").arg(utf8.size()).arg(deviceConfig));
  }

  // -----------------------------------------------------------------------------------------------
  // Step 4: record. `perfetto --txt` records synchronously until the config's duration_ms expires,
  // which is why this call blocks for that long and why cancellation cannot cut it short: the
  // device protocol serialises commands onto one thread, so there is no channel to send a stop
  // signal while this command is in flight.
  //
  // Perfetto runs under a more restricted SELinux domain than `adb shell`. On this Pixel 3 its
  // perfetto_cmd domain cannot open the adb-pushed config, even though adb shell can access the
  // staging directory, so the config is passed through stdin. Conversely, SELinux forbids the shell
  // process from creating files in /data/misc/perfetto-traces; Perfetto itself must create the
  // output in that platform-managed directory.
  // -----------------------------------------------------------------------------------------------
  {
    const QString command = QStringLiteral("cat %1 | perfetto --txt -c - -o %2")
                                .arg(ShellQuote(deviceConfig), ShellQuote(deviceTrace));

    Log(result, QStringLiteral("recording: %1 (blocking for the configured duration)")
                    .arg(command));

    rdcstr out;
    const ResultDetails rec = protocol->ExecCommand(urlStr,
                                                    command.toUtf8().data(), &out);

    const QString recOut = ToQStringDisplay(out);
    if(!recOut.trimmed().isEmpty())
      Log(result, QStringLiteral("perfetto output: %1").arg(Summarise(recOut)));

    if(!rec.OK())
    {
      Log(result, QStringLiteral("recording failed: %1").arg(ToQStringDisplay(rec.Message())));
      Fail(result, QStringLiteral("record trace on device"), ToQStringDisplay(rec.Message()));
      return result;
    }

    // perfetto can exit successfully while writing nothing when the config is rejected for a
    // non-fatal reason, so the existence and size of the output are checked rather than assumed.
    rdcstr stat;
    const ResultDetails ls = protocol->ExecCommand(
        urlStr, QStringLiteral("ls -l %1").arg(ShellQuote(deviceTrace)), &stat);

    if(!ls.OK())
    {
      Log(result, QStringLiteral("trace stat failed: %1").arg(ToQStringDisplay(ls.Message())));
      Fail(result, QStringLiteral("verify recorded trace"),
           QStringLiteral("perfetto reported success but no trace was produced.\n%1")
               .arg(ToQStringDisplay(ls.Message())));
      return result;
    }

    Log(result, QStringLiteral("recorded: %1").arg(Summarise(ToQStringDisplay(stat), 300)));
  }

  // -----------------------------------------------------------------------------------------------
  // Step 5: pull the protobuf back. It is kept even if conversion fails, so a recording is never
  // lost to a converter problem.
  // -----------------------------------------------------------------------------------------------
  {
    const ResultDetails pull = protocol->PullFile(
        urlStr, deviceTrace.toUtf8().data(), localTrace.toUtf8().data());

    if(!pull.OK())
    {
      Log(result, QStringLiteral("pull failed: %1").arg(ToQStringDisplay(pull.Message())));
      Fail(result, QStringLiteral("pull trace from device"), ToQStringDisplay(pull.Message()));
      return result;
    }

    const QFileInfo info(localTrace);
    Log(result, QStringLiteral("pulled %1 bytes to %2")
                    .arg(info.size())
                    .arg(localTrace));

    if(info.size() == 0)
    {
      Fail(result, QStringLiteral("pull trace from device"),
           QStringLiteral("the trace file transferred empty: %1").arg(localTrace));
      return result;
    }
  }

  // The device staging directory and trace are no longer needed. Failure to remove either is
  // non-fatal because the capture itself already succeeded.
  {
    rdcstr ignored;
    const ResultDetails rm = protocol->ExecCommand(
        urlStr, QStringLiteral("rm -rf %1; rm -f %2")
                    .arg(ShellQuote(deviceDir), ShellQuote(deviceTrace)),
        &ignored);
    if(!rm.OK())
      Log(result, QStringLiteral("cleanup of %1 and %2 failed (non-fatal): %3")
                      .arg(deviceDir, deviceTrace, ToQStringDisplay(rm.Message())));
    else
      Log(result, QStringLiteral("removed device staging directory and trace"));
  }

  // -----------------------------------------------------------------------------------------------
  // Step 6: convert the protobuf to a timeline the importer can read. `perfetto` itself writes a
  // protobuf, which the importer does not parse, so the conversion is required rather than optional.
  // -----------------------------------------------------------------------------------------------
  result.traceconvPath = FindTraceconv(opts, result);

  if(result.traceconvPath.isEmpty())
  {
    Fail(result, QStringLiteral("locate traceconv"),
         QStringLiteral("The trace was recorded and pulled to:\n%1\n\n"
                        "Converting it needs Perfetto's `traceconv`. Set its path in the capture "
                        "dialog, install it on PATH, or point RenderDoc at a Snapdragon Profiler "
                        "installation that bundles it.")
             .arg(localTrace));
    return result;
  }

  // `systrace` text is preferred over `json`: it is the same event stream without the JSON
  // wrapper, so the file is slightly smaller and needs no unescaping, but the reason that matters is
  // that it is the shape the importer parses natively. `json` remains as the fallback for traceconv
  // builds that do not offer the sub-command.
  const QString textPath = QDir(localDir).absoluteFilePath(QStringLiteral("capture.systrace.txt"));
  const QString jsonPath = QDir(localDir).absoluteFilePath(QStringLiteral("capture.json"));

  QString importPath;
  QStringList convertErrors;
  {
    const struct
    {
      const char *subcommand;
      const QString *output;
    } attempts[] = {
        {"systrace", &textPath},
        {"json", &jsonPath},
    };

    for(const auto &attempt : attempts)
    {
      bool ok = false;
      const QString toolOut =
          RunLocalTool(result.traceconvPath,
                       {QLatin1String(attempt.subcommand), localTrace, *attempt.output},
                       10 * 60 * 1000, ok);

      if(!toolOut.trimmed().isEmpty())
        Log(result, QStringLiteral("traceconv %1 output: %2")
                        .arg(QLatin1String(attempt.subcommand), Summarise(toolOut)));

      const qint64 size =
          QFileInfo::exists(*attempt.output) ? QFileInfo(*attempt.output).size() : -1;

      if(ok && size > 0)
      {
        importPath = *attempt.output;
        Log(result, QStringLiteral("converted %1 bytes to %2 (traceconv %3)")
                        .arg(size)
                        .arg(importPath, QLatin1String(attempt.subcommand)));
        break;
      }

      convertErrors << QStringLiteral("`traceconv %1` produced no output: %2")
                           .arg(QLatin1String(attempt.subcommand), Summarise(toolOut));
    }
  }

  if(importPath.isEmpty())
  {
    // An older or newer `traceconv` may expect different arguments. What each attempt printed is
    // reported verbatim so that a mismatch is visible instead of presenting a generic error.
    Fail(result, QStringLiteral("convert trace to an importable timeline"),
         QStringLiteral("%1\n\nThe recorded protobuf is retained at:\n%2")
             .arg(convertErrors.join(QStringLiteral("\n")), localTrace));
    return result;
  }

  result.localTimelinePath = importPath;

  result.success = true;
  Log(result, QStringLiteral("capture complete"));
  return result;
}

void TraceCaptureJob::Start(QObject *owner, IDeviceProtocolController *protocol, const QString &url,
                            const TraceCaptureOptions &opts, Callback done)
{
  QPointer<QObject> guard(owner);

  LambdaThread *thread = new LambdaThread([protocol, url, opts, done, guard]() {
    TraceCaptureResult result = RunTraceCapture(protocol, url, opts);
    PersistCaptureLog(result);

    // The caller's widget may have been destroyed while the device was recording, in which case
    // the result still exists (it is local evidence) but there is nobody to deliver it to.
    if(!guard)
      return;

    GUIInvoke::call(guard, [done, result]() { done(result); });
  });

  thread->setName(lit("TraceCapture"));
  thread->selfDelete(true);
  thread->start();
}

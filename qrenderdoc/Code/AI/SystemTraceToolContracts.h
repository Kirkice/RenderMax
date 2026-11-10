// Shared file-side tool contracts. No replay or Widgets dependency.
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <climits>
namespace MCP
{
template <typename Add> void RegisterTraceFileTools(Add add)
{
  auto string = [](bool nonempty = false) { QJsonObject s{{QStringLiteral("type"), QStringLiteral("string")}}; if(nonempty) s[QStringLiteral("minLength")] = 1; return s; };
  auto integer = [](int min, int max) { return QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}, {QStringLiteral("minimum"), min}, {QStringLiteral("maximum"), max}}; };
  // Reading a trace is two different jobs with different sizes: "what is in this trace" is a summary,
  // "what happened in this lane between these times" is a query. One tool with defaulted, bounded
  // options serves both without a second round trip, and without a reply large enough to be useless.
  const QJsonObject traceAnalyze{
      {QStringLiteral("filePath"), string(true)},
      {QStringLiteral("beginSec"), QJsonObject{{QStringLiteral("type"), QStringLiteral("number")}}},
      {QStringLiteral("endSec"), QJsonObject{{QStringLiteral("type"), QStringLiteral("number")}}},
      {QStringLiteral("trackFilter"), string()},
      {QStringLiteral("seriesFilter"), string()},
      {QStringLiteral("includeSpans"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}}},
      {QStringLiteral("includeSamples"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}}},
      {QStringLiteral("maxSpansPerTrack"), integer(1, 5000)},
      {QStringLiteral("maxSamplesPerSeries"), integer(2, 5000)}};
  add("renderdoc_analyzeSystemTrace", "Import a local System Trace and return counters, tracks, scheduler thread time, frame GPU time, GPU counter accounting, unmodelled event types, warnings and the supported ftrace event inventory. Span and counter contents are omitted by default; set includeSpans/includeSamples, and narrow with beginSec/endSec or the name filters.", traceAnalyze, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_saveSystemTrace", "Atomically save a self-contained versioned .rdt archive from an existing importable trace or .rdt. Existing destinations require overwrite true.",
      QJsonObject{{QStringLiteral("filePath"), string(true)}, {QStringLiteral("destinationPath"), string(true)},
        {QStringLiteral("overwrite"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}}}},
      QJsonArray{QStringLiteral("filePath"), QStringLiteral("destinationPath")}, false);
  QJsonObject query = traceAnalyze;
  for(const char *key : {"pid", "tid", "cpu", "offset", "limit", "frameIndex", "buckets"})
    query[QString::fromLatin1(key)] = integer(0, QString::fromLatin1(key) == QStringLiteral("limit") ? 5000 : INT_MAX);
  for(const char *key : {"compareBeginSec", "compareEndSec", "minDurationSec", "frameNumber", "frameBudgetMs"})
    query[QString::fromLatin1(key)] = QJsonObject{{QStringLiteral("type"), QStringLiteral("number")}};
  for(const char *key : {"threadFilter", "groupFilter", "sortBy"})
    query[QString::fromLatin1(key)] = string();
  add("renderdoc_getSystemTraceCapabilities", "Observed CPU/GPU capabilities and data gaps; supported events are not proof of occurrence.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_querySystemTraceSeries", "Paged metric queries with window statistics, step residency and optional bounded min/max envelopes.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_querySystemTraceTracks", "Paged span queries, clipped overlap duration, original timestamps and source fields.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_getSystemTraceEventInventory", "Supported ftrace schemas, observed trace-wide event counts and unmodelled event counts.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTraceCpu", "CPU scheduler coverage, per-core busy/idle and ranked thread costs.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_getSystemTraceCpuUtilization", "Per-core observed scheduler utilization and per-thread one-core window utilization.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_getSystemTraceThreadTimes", "Window-clipped running, runnable and sleeping times per thread.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTraceScheduling", "Scheduler intervals and runnable waiting; missing causal provenance is explicitly reported.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTraceWakeups", "Observed waking/wakeup to schedule-in latency, unmatched endpoints and runnable interval distributions.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTraceCpuFrequency", "CPU frequency sample and time-weighted residency statistics.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTraceCpuIdle", "Observed CPU idle series and state residency.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTraceGpu", "Validated GPU execution union and accounting, without inventing idle from missing events.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_getSystemTraceGpuUtilization", "GPU known busy lower bound; uncovered time is not asserted idle.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_getSystemTraceGpuBatches", "Paged GPU command batch spans with source evidence fields.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTraceGpuQueue", "Inflight queue samples and typed anchored per-batch submit-to-start latency; missing anchors remain unknown.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTraceGpuWaits", "Paged GPU fence waits and window overlap durations.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTraceGpuFrequency", "GPU frequency and clock level statistics and residency.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTraceBandwidth", "Bandwidth statistics grouped by source device; CPU and GPU masters remain distinct.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTracePower", "Power sample statistics; no energy extrapolation over unknown sampling gaps.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTraceThermal", "Temperature samples and thermal trip events; correlations are not causal throttling proof.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTracePowerStates", "Observed GPU power state intervals and state residency.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_getSystemTraceFrames", "Frame marker intervals, declared frame identity and measured GPU busy.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTraceFrame", "Select frameIndex or declared frameNumber and inspect CPU/GPU within that interval.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_analyzeSystemTraceJank", "Over-budget marker intervals; marker spacing is not presentation timing.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_compareSystemTraceWindows", "Compare CPU and GPU statistics for two explicit windows in one trace.", query, QJsonArray{QStringLiteral("filePath")}, true);
  add("renderdoc_buildSystemTraceReport", "Window CPU/GPU, metrics and frame evidence with limitations, not speculative causal diagnoses.", query, QJsonArray{QStringLiteral("filePath")}, true);
}
}

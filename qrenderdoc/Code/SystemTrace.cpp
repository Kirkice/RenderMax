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

#include "SystemTrace.h"

#include <algorithm>
#include <cstring>

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>

// [观测] System-trace import provenance (planning document section 8.9.3).
//
// Reverse-engineering the Snapdragon Profiler host and device components established that SDP's
// own system-trace backend is Perfetto/Systrace, not a bespoke format:
//   * rodata in the SDP host assemblies references `TraceImportExport.TraceImport.BUFFER_TYPE_
//     SYSTRACE_DATA`, `pluginPerfetto.dll`, `SystraceProcessor.dll`, and `traceconv-win-amd64.exe`.
//   * SDP therefore records a Perfetto protobuf trace and shells out to `traceconv` to render it
//     (including a `json` sub-command) for interchange.
// This file does NOT parse the Perfetto protobuf and does NOT link Perfetto. It parses the
// line-delimited JSON that the `traceconv ... json`-style export produces, which keeps the
// importer dependency-free while still consuming the real exporter's output shape.
//
// The deprecated `PeakJSONL` name that used to label formatName described an invented interchange
// that SDP never emits; it has been replaced with the actual provenance string so downstream
// diagnostics report where a trace really came from.

void TraceSeries::Sort()
{
  std::sort(samples.begin(), samples.end(),
            [](const TraceSample &a, const TraceSample &b) { return a.timeSec < b.timeSec; });
}

double TraceSeries::MinValue() const
{
  if(samples.isEmpty())
    return 0.0;

  double v = samples[0].value;
  for(const TraceSample &s : samples)
    v = s.value < v ? s.value : v;
  return v;
}

double TraceSeries::MaxValue() const
{
  if(samples.isEmpty())
    return 0.0;

  double v = samples[0].value;
  for(const TraceSample &s : samples)
    v = s.value > v ? s.value : v;
  return v;
}

bool TraceSeries::SampleAt(double traceTimeSec, double &outValue) const
{
  if(samples.isEmpty())
    return false;

  // The series is expected to be sorted; guard against an unsorted import anyway by scanning
  // when the endpoints don't bracket the query in the expected direction.
  const bool sortedAscending = samples.first().timeSec <= samples.last().timeSec;

  if(!sortedAscending)
  {
    if(samples.first().timeSec <= samples.last().timeSec)
      return false;

    // Linear scan fallback for a descending series.
    if(traceTimeSec <= samples.first().timeSec)
    {
      outValue = samples.first().value;
      return true;
    }
    if(traceTimeSec >= samples.last().timeSec)
    {
      outValue = samples.last().value;
      return true;
    }
    for(int i = 0; i + 1 < samples.size(); i++)
    {
      const TraceSample &a = samples[i];
      const TraceSample &b = samples[i + 1];
      // Descending: a.timeSec >= b.timeSec
      if(traceTimeSec <= a.timeSec && traceTimeSec >= b.timeSec)
      {
        const double span = a.timeSec - b.timeSec;
        const double t = span > 0.0 ? (a.timeSec - traceTimeSec) / span : 0.0;
        outValue = a.value + (b.value - a.value) * t;
        return true;
      }
    }
    outValue = samples.last().value;
    return true;
  }

  if(traceTimeSec <= samples.first().timeSec)
  {
    outValue = samples.first().value;
    return true;
  }
  if(traceTimeSec >= samples.last().timeSec)
  {
    outValue = samples.last().value;
    return true;
  }

  // Binary search for the first sample with timeSec >= traceTimeSec.
  int lo = 0;
  int hi = samples.size() - 1;
  while(lo < hi)
  {
    const int mid = (lo + hi) / 2;
    if(samples[mid].timeSec < traceTimeSec)
      lo = mid + 1;
    else
      hi = mid;
  }

  const TraceSample &b = samples[lo];
  const TraceSample &a = samples[lo - 1];

  // A level series only ever takes the values it was sampled at, so the value at any time is the
  // one from the last sample at or before it.
  if(step)
  {
    outValue = a.value;
    return true;
  }

  const double span = b.timeSec - a.timeSec;
  const double t = span > 0.0 ? (traceTimeSec - a.timeSec) / span : 0.0;
  outValue = a.value + (b.value - a.value) * t;
  return true;
}

const TraceSeries *SystemTrace::FindSeries(const QString &name) const
{
  for(const TraceSeries &s : series)
  {
    if(s.name == name)
      return &s;
  }
  return NULL;
}

void SystemTrace::TimeExtent(double &startSec, double &endSec) const
{
  double minT = 0.0;
  double maxT = 0.0;
  bool any = false;

  const auto extend = [&](double t) {
    if(!any)
    {
      minT = maxT = t;
      any = true;
    }
    else
    {
      minT = t < minT ? t : minT;
      maxT = t > maxT ? t : maxT;
    }
  };

  for(const TraceSeries &s : series)
  {
    for(const TraceSample &sample : s.samples)
      extend(sample.timeSec);
  }

  // Spans define the visible extent of a lane timeline just as samples do for a counter lane, so
  // they must be included: a trace holding only slices would otherwise report a zero span and be
  // drawn as an empty timeline.
  for(const TraceTrack &track : tracks)
  {
    for(const TraceSpan &span : track.spans)
    {
      extend(span.beginSec);
      extend(span.endSec);
    }
  }

  for(const TraceFrameBoundary &b : frameBoundaries)
    extend(b.timeSec);

  startSec = minT;
  endSec = maxT;

  if(!any)
  {
    startSec = 0.0;
    endSec = 0.0;
  }
}

double SystemTrace::SpanSec() const
{
  double start = 0.0;
  double end = 0.0;

  TimeExtent(start, end);

  return end > start ? (end - start) : 0.0;
}

int SystemTrace::SpanCount() const
{
  int count = 0;
  for(const TraceTrack &track : tracks)
    count += track.spans.size();

  return count;
}

bool SystemTrace::HasDeclaredFrameNumbers() const
{
  for(const TraceFrameBoundary &b : frameBoundaries)
  {
    if(b.source.isEmpty())
      return true;
  }

  return false;
}

SystemTrace ImportSystemTraceJsonLines(const QString &path)
{
  SystemTrace trace;
  trace.source = path;

  // [观测] The interchange is a `traceconv`-style line-JSON export of a Perfetto/Systrace trace
  // (planning document section 8.9.3), not the invented "PeakJSONL" envelope. The provenance
  // string is set here so the viewer and correlation report a truthful origin. The parser below
  // is intentionally format-agnostic within that envelope: it accepts `header`, `series`,
  // `sample` and `frame` records and tolerates unknown ones.
  trace.formatName = QStringLiteral("Perfetto/Systrace (traceconv json)");

  QFile file(path);
  if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    trace.warnings << QObject::tr("Could not open trace file: %1").arg(path);
    return trace;
  }

  int lineNumber = 0;
  int lineErrors = 0;
  bool sawVersion = false;

  while(!file.atEnd())
  {
    const QByteArray raw = file.readLine();
    lineNumber++;

    const QByteArray line = raw.trimmed();
    if(line.isEmpty() || line.startsWith('#'))
      continue;

    QJsonParseError err = {};
    const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
    if(err.error != QJsonParseError::NoError || !doc.isObject())
    {
      lineErrors++;
      continue;
    }

    const QJsonObject obj = doc.object();
    const QString type = obj.value(QStringLiteral("type")).toString();

    if(type == QStringLiteral("header"))
    {
      if(obj.contains(QStringLiteral("formatVersion")))
      {
        const int version = obj.value(QStringLiteral("formatVersion")).toInt();
        if(version != 1)
          trace.warnings << QObject::tr("Trace format version %1 is not the supported version 1").arg(version);
        else
          sawVersion = true;
      }
      if(obj.contains(QStringLiteral("timeUnit")))
        trace.timeUnit = obj.value(QStringLiteral("timeUnit")).toString();
      if(obj.contains(QStringLiteral("source")))
        trace.source = obj.value(QStringLiteral("source")).toString();
      // An explicit declaration lets correlation trust trace time without deriving an offset.
      if(obj.value(QStringLiteral("clockDomain")).toString() == QStringLiteral("shared"))
        trace.sharedClock = true;
    }
    else if(type == QStringLiteral("series"))
    {
      TraceSeries s;
      s.name = obj.value(QStringLiteral("name")).toString();
      s.unit = obj.value(QStringLiteral("unit")).toString();

      if(s.name.isEmpty())
      {
        lineErrors++;
        continue;
      }

      const QJsonArray samples = obj.value(QStringLiteral("samples")).toArray();
      s.samples.reserve(samples.size());
      for(const QJsonValue &sampleVal : samples)
      {
        const QJsonArray pair = sampleVal.toArray();
        if(pair.size() < 2)
          continue;
        TraceSample sample;
        sample.timeSec = pair.at(0).toDouble();
        sample.value = pair.at(1).toDouble();
        s.samples.push_back(sample);
      }

      if(s.samples.isEmpty())
      {
        trace.warnings << QObject::tr("Series '%1' contained no samples").arg(s.name);
        continue;
      }

      s.Sort();
      trace.series.push_back(s);
    }
    else if(type == QStringLiteral("sample"))
    {
      const QString name = obj.value(QStringLiteral("series")).toString();
      int idx = -1;
      for(int i = 0; i < trace.series.size(); i++)
      {
        if(trace.series[i].name == name)
        {
          idx = i;
          break;
        }
      }

      if(idx < 0)
      {
        TraceSeries s;
        s.name = name;
        s.unit = obj.value(QStringLiteral("unit")).toString();
        idx = trace.series.size();
        trace.series.push_back(s);
      }

      TraceSample sample;
      sample.timeSec = obj.value(QStringLiteral("t")).toDouble();
      sample.value = obj.value(QStringLiteral("v")).toDouble();
      trace.series[idx].samples.push_back(sample);
    }
    else if(type == QStringLiteral("frame"))
    {
      TraceFrameBoundary b;
      b.frameNumber = (uint32_t)obj.value(QStringLiteral("frameNumber")).toInt();
      b.timeSec = obj.value(QStringLiteral("t")).toDouble();
      trace.frameBoundaries.push_back(b);
    }
    else
    {
      // Unknown record types are ignored but not hidden: the first few are surfaced.
      if(lineErrors < 8)
        trace.warnings << QObject::tr("Ignored unknown trace record type '%1' at line %2")
                              .arg(type)
                              .arg(lineNumber);
      lineErrors++;
    }
  }

  if(!sawVersion && !trace.series.isEmpty())
    trace.warnings << QObject::tr("Trace did not declare 'formatVersion': assuming version 1");

  // Samples appended via 'sample' records need a final sort.
  for(TraceSeries &s : trace.series)
    s.Sort();

  if(lineErrors > 8)
    trace.warnings << QObject::tr("%1 additional malformed or unknown lines were skipped")
                          .arg(lineErrors - 8);

  if(trace.series.isEmpty() && trace.frameBoundaries.isEmpty())
  {
    trace.warnings << QObject::tr("Trace contained no usable series or frame boundaries");
    return trace;
  }

  if(trace.frameBoundaries.isEmpty())
    trace.warnings << QObject::tr(
        "Trace contained no frame boundaries: correlation to the capture will be limited to "
        "absolute time");

  if(trace.timeUnit.isEmpty())
    trace.warnings << QObject::tr("Trace did not declare a time unit: assuming seconds");

  trace.valid = true;
  return trace;
}

namespace
{
// Chrome Trace timestamps are microseconds in the device clock.
const double MicrosecondsToSeconds = 1.0e-6;

struct ChromeEventStats
{
  int events = 0;
  int counters = 0;
  int spans = 0;
  int unmappedPhase = 0;
  int missingTimestamp = 0;
  int malformed = 0;
  // Counter events that parsed but carried no named value, so there is nothing to caption a series
  // with. Counted separately from `malformed` because the record itself was well formed.
  int unusableCounters = 0;
  int missingDuration = 0;
  int unterminatedSpans = 0;
  int reversedSpans = 0;
  int metadataApplied = 0;
  int metadataUnusable = 0;
  int framesFromArgs = 0;
  int framesFromName = 0;
  // Totals for the Systrace (ftrace text) branch, kept separate from the Chrome event counts so a
  // warning can say which of the two forms contributed.
  int systraceEvents = 0;
  int systraceSpans = 0;
  int systraceCounterSamples = 0;
  int systraceUnmodelled = 0;
  // How many atrace marks (`tracing_mark_write`) were read. Zero in a trace that also carries
  // scheduler data is the signature of atrace not being enabled on the device, which looks exactly
  // like a trace where the app drew nothing.
  int atraceMarks = 0;
  int derivedFrames = 0;
  int droppedClockSeries = 0;
};

// A trace can carry dozens of clocks. Every one of them is a real measurement, but a ceiling keeps
// the metric lane count finite; the ones left out are reported rather than silently dropped.
const int MaxClockSeries = 32;

// One (pid,tid) lane as read from the file, before names are resolved and before nesting depth is
// computed. Lanes are created on demand in first-seen order and then reordered for display.
struct RawLane
{
  int pid = -1;
  int tid = -1;
  QVector<TraceSpan> spans;
  // True when the lane holds atrace (userspace) marks rather than scheduler slices. Only used to
  // caption it as a process lane when no thread name was declared for the id.
  bool atrace = false;
};

// Accumulates the whole file. Metadata is kept here rather than applied immediately because
// `thread_name` / `process_name` records routinely follow the spans that they caption.
struct ChromeTraceBuilder
{
  QVector<TraceSeries> series;
  QVector<TraceFrameBoundary> frames;
  QVector<RawLane> lanes;
  QHash<quint64, int> laneIndex;
  QHash<int, QString> threadNames;
  QHash<int, QString> processNames;
  // Begin/end ('B' then 'E') spans still open, per lane, outermost last.
  QHash<quint64, QVector<TraceSpan>> openSpans;
  // Per-CPU lanes, indexed by CPU number. Filled from the scheduler events in the Systrace text.
  QVector<RawLane> cpuLanes;
  // Per-CPU interrupt and softirq lanes, also indexed by CPU number. Interrupts are not threads, so
  // they cannot be drawn on a thread lane: putting them here is what keeps a CPU's own row honest
  // while still showing that the CPU was taken away from the thread it was running.
  QVector<RawLane> irqLanes;
  QVector<RawLane> softirqLanes;
  // tid -> the comm the scheduler last called it. This is the only thread identity a raw systrace
  // export carries: it has no `thread_name` metadata at all, and the comm travels with every
  // sched_switch record.
  QHash<int, QString> commByTid;
  // Ftrace event types (and atrace mark kinds) that produced no lane or metric, with the number of
  // records seen for each. Reported in one warning so the panel can say what it omitted.
  QHash<QString, int> unmodelledEvents;
  ChromeEventStats stats;
};

// A lane is identified by the pair the trace itself declares. A missing id is not invented: it is
// folded into one explicitly "unknown" lane so those events stay visible instead of being dropped.
quint64 LaneKey(int pid, int tid)
{
  const quint32 p = pid < 0 ? 0xFFFFFFFFu : (quint32)pid;
  const quint32 t = tid < 0 ? 0xFFFFFFFFu : (quint32)tid;
  return ((quint64)p << 32) | (quint64)t;
}

RawLane &LaneFor(ChromeTraceBuilder &builder, int pid, int tid)
{
  const quint64 key = LaneKey(pid, tid);

  const auto it = builder.laneIndex.constFind(key);
  if(it != builder.laneIndex.constEnd())
    return builder.lanes[*it];

  RawLane lane;
  lane.pid = pid;
  lane.tid = tid;

  const int index = builder.lanes.size();
  builder.lanes.push_back(lane);
  builder.laneIndex.insert(key, index);

  return builder.lanes[index];
}

// `args` values are retained verbatim for the detail pane. Scalars are rendered the way the source
// wrote them; nested objects and arrays are rendered as compact JSON rather than being dropped,
// because the value is evidence even when it is not a scalar.
QString FormatArgValue(const QJsonValue &val)
{
  if(val.isString())
    return val.toString();
  if(val.isDouble())
    return QString::number(val.toDouble(), 'g', 10);
  if(val.isBool())
    return val.toBool() ? QStringLiteral("true") : QStringLiteral("false");
  if(val.isNull() || val.isUndefined())
    return QStringLiteral("null");

  const QJsonDocument doc =
      val.isArray() ? QJsonDocument(val.toArray()) : QJsonDocument(val.toObject());
  return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

QStringList EventFields(const QJsonObject &obj)
{
  QStringList fields;

  const QJsonObject args = obj.value(QStringLiteral("args")).toObject();
  for(auto it = args.begin(); it != args.end(); ++it)
    fields << QStringLiteral("%1 = %2").arg(it.key(), FormatArgValue(it.value()));

  return fields;
}

// Depth from interval containment: spans already open when this one starts are its ancestors. The
// stack only ever holds intervals that are still open at the current start time, so siblings and
// later spans never contribute. Sorting outer-before-inner (longer span first on equal start) keeps
// the containment relationship consistent with what a reader sees in the trace.
void AssignNestingDepth(QVector<TraceSpan> &spans)
{
  std::sort(spans.begin(), spans.end(), [](const TraceSpan &a, const TraceSpan &b) {
    if(a.beginSec != b.beginSec)
      return a.beginSec < b.beginSec;
    return a.endSec > b.endSec;
  });

  QVector<double> openEnds;
  for(TraceSpan &span : spans)
  {
    while(!openEnds.isEmpty() && openEnds.last() <= span.beginSec)
      openEnds.removeLast();

    span.depth = openEnds.size();
    openEnds.push_back(span.endSec);
  }
}

// Extracts an explicit frame index from an event. Returns -1 when the event does not describe a
// frame boundary. Two forms are accepted, in order of authority:
//   1. an integer in args under frameNumber / frame_number / frame
//   2. a slice name of the exact form "Frame <n>"
// `fromName` reports which form matched so the caller can record the weaker provenance.
int64_t FrameIndexFromEvent(const QJsonObject &obj, bool &fromName)
{
  fromName = false;

  const QJsonObject args = obj.value(QStringLiteral("args")).toObject();
  const char *argKeys[] = {"frameNumber", "frame_number", "frame"};
  for(const char *key : argKeys)
  {
    const QJsonValue val = args.value(QLatin1String(key));
    if(val.isDouble())
      return (int64_t)val.toDouble();
  }

  const QString name = obj.value(QStringLiteral("name")).toString();
  if(name.startsWith(QStringLiteral("Frame "), Qt::CaseInsensitive))
  {
    bool ok = false;
    const int64_t index = name.mid(6).trimmed().toLongLong(&ok);
    if(ok)
    {
      fromName = true;
      return index;
    }
  }

  return -1;
}

// Collects a counter event's values. Chrome counters carry a name per value in `args`, which keeps
// distinct counters in one event separate rather than merged into one series.
void AddCounterEvent(const QJsonObject &obj, QVector<TraceSeries> &series, ChromeEventStats &stats)
{
  const double timeSec = obj.value(QStringLiteral("ts")).toDouble() * MicrosecondsToSeconds;
  const QString category = obj.value(QStringLiteral("cat")).toString();
  const QJsonObject args = obj.value(QStringLiteral("args")).toObject();

  if(args.isEmpty())
  {
    // A counter with no named values would need an invented name to be displayed, so it is
    // counted rather than guessed at.
    stats.unusableCounters++;
    return;
  }

  for(auto it = args.begin(); it != args.end(); ++it)
  {
    if(!it.value().isDouble())
      continue;

    const QString name = category.isEmpty() ? it.key() : category + QStringLiteral("/") + it.key();

    int idx = -1;
    for(int i = 0; i < series.size(); i++)
    {
      if(series[i].name == name)
      {
        idx = i;
        break;
      }
    }

    if(idx < 0)
    {
      TraceSeries s;
      s.name = name;
      // Chrome counters do not declare a unit. Leaving it empty is the truthful choice.
      idx = series.size();
      series.push_back(s);
    }

    TraceSample sample;
    sample.timeSec = timeSec;
    sample.value = it.value().toDouble();
    series[idx].samples.push_back(sample);
  }

  stats.counters++;
}

// A lane id as the trace declares it. Chrome traces put pid/tid at the event's top level; the
// Perfetto exporter always writes both, but a trace without them still has to produce a lane
// rather than losing the event.
void EventLane(const QJsonObject &obj, int &pid, int &tid)
{
  const QJsonValue pidVal = obj.value(QStringLiteral("pid"));
  const QJsonValue tidVal = obj.value(QStringLiteral("tid"));

  pid = pidVal.isDouble() ? (int)pidVal.toDouble() : -1;
  tid = tidVal.isDouble() ? (int)tidVal.toDouble() : -1;
}

void ConsumeMetadata(const QJsonObject &obj, ChromeTraceBuilder &builder)
{
  const QString name = obj.value(QStringLiteral("name")).toString();
  const QJsonObject args = obj.value(QStringLiteral("args")).toObject();

  int pid = -1;
  int tid = -1;
  EventLane(obj, pid, tid);

  // Perfetto writes the captioned name in args.name; some producers use args directly as a
  // single-field object instead. Both are read, and neither is invented if absent.
  QString value;
  if(args.contains(QStringLiteral("name")))
    value = args.value(QStringLiteral("name")).toString();
  else if(args.size() == 1)
    value = args.begin().value().toString();

  if(value.isEmpty())
  {
    builder.stats.metadataUnusable++;
    return;
  }

  if(name == QStringLiteral("thread_name"))
  {
    if(tid >= 0)
    {
      builder.threadNames.insert(tid, value);

      // `traceconv` emits only `thread_name` records, so this is the only process name a trace may
      // carry. The thread whose tid equals the pid IS the process's main thread, so its name is the
      // best identity the source declared. An explicit `process_name` record always wins over it.
      if(tid == pid && !builder.processNames.contains(pid))
        builder.processNames.insert(pid, value);

      builder.stats.metadataApplied++;
    }
    else
    {
      builder.stats.metadataUnusable++;
    }
    return;
  }

  if(name == QStringLiteral("process_name"))
  {
    if(pid >= 0)
    {
      builder.processNames.insert(pid, value);
      builder.stats.metadataApplied++;
    }
    else
    {
      builder.stats.metadataUnusable++;
    }
    return;
  }

  // Any other metadata key (labels, sort indices, ...) is not modelled. It is not counted as a
  // problem because nothing downstream can display it.
}

void ConsumeChromeEvent(const QJsonObject &obj, ChromeTraceBuilder &builder)
{
  builder.stats.events++;

  const QString phase = obj.value(QStringLiteral("ph")).toString();

  if(phase == QStringLiteral("C"))
  {
    AddCounterEvent(obj, builder.series, builder.stats);
    return;
  }

  if(phase == QStringLiteral("M"))
  {
    // Metadata carries no timestamp; it captions lanes that are resolved after the file is read.
    ConsumeMetadata(obj, builder);
    return;
  }

  if(!obj.contains(QStringLiteral("ts")))
  {
    builder.stats.malformed++;
    return;
  }

  const double timeSec = obj.value(QStringLiteral("ts")).toDouble() * MicrosecondsToSeconds;

  // Frame detection runs for every timestamped event, exactly as before, so a span that also
  // declares a frame index still contributes its boundary.
  bool fromName = false;
  const int64_t frameIndex = FrameIndexFromEvent(obj, fromName);

  if(frameIndex >= 0)
  {
    TraceFrameBoundary b;
    b.frameNumber = (uint32_t)frameIndex;
    b.timeSec = timeSec;
    builder.frames.push_back(b);

    if(fromName)
      builder.stats.framesFromName++;
    else
      builder.stats.framesFromArgs++;
  }

  int pid = -1;
  int tid = -1;
  EventLane(obj, pid, tid);

  if(phase == QStringLiteral("X"))
  {
    // A complete span. `dur` is required: without it there is no interval to draw, and inventing
    // one (e.g. zero-length) would misrepresent the trace.
    const QJsonValue durVal = obj.value(QStringLiteral("dur"));
    if(!durVal.isDouble())
    {
      builder.stats.missingDuration++;
      return;
    }

    TraceSpan span;
    span.beginSec = timeSec;
    span.endSec = timeSec + durVal.toDouble() * MicrosecondsToSeconds;
    span.name = obj.value(QStringLiteral("name")).toString();
    span.category = obj.value(QStringLiteral("cat")).toString();
    span.fields = EventFields(obj);

    if(span.endSec < span.beginSec)
    {
      // Cannot happen for a non-negative dur, but a negative dur is a malformed source, not a
      // reason to draw an inverted box.
      builder.stats.reversedSpans++;
      return;
    }

    LaneFor(builder, pid, tid).spans.push_back(span);
    builder.stats.spans++;
    return;
  }

  if(phase == QStringLiteral("B"))
  {
    // Begin: pushed onto this lane's stack. Its end time is unknown until the matching 'E'.
    TraceSpan span;
    span.beginSec = timeSec;
    span.endSec = timeSec;
    span.name = obj.value(QStringLiteral("name")).toString();
    span.category = obj.value(QStringLiteral("cat")).toString();
    span.fields = EventFields(obj);

    // Ensure the lane exists even if the 'E' never arrives, so the begin is still evident.
    LaneFor(builder, pid, tid);
    builder.openSpans[LaneKey(pid, tid)].push_back(span);
    return;
  }

  if(phase == QStringLiteral("E"))
  {
    QVector<TraceSpan> &open = builder.openSpans[LaneKey(pid, tid)];

    if(open.isEmpty())
    {
      // An 'E' without a 'B': there is no interval to close.
      builder.stats.unterminatedSpans++;
      return;
    }

    TraceSpan span = open.takeLast();
    span.endSec = timeSec;

    if(span.endSec < span.beginSec)
    {
      builder.stats.reversedSpans++;
      return;
    }

    LaneFor(builder, pid, tid).spans.push_back(span);
    builder.stats.spans++;
    return;
  }

  // Instant ('i'), async ('b'/'n'/'e'), flow ('s'/'t'/'f'), object ('N'/'O'/'D') and legacy phases
  // have no duration to draw as a lane interval, so they are counted rather than coerced.
  builder.stats.unmappedPhase++;
}

// ---------------------------------------------------------------------------------------------
// Systrace (ftrace text) parsing
// ---------------------------------------------------------------------------------------------
//
// `traceconv` embeds the device's kernel trace verbatim in the Chrome trace document's
// `systemTraceEvents` member and leaves `traceEvents` holding little more than metadata. Its lines
// use the classic systrace layout:
//
//   <...>-4226  ( 4203) [003] .... 2343.167670: sched_switch: prev_comm=UnityMain prev_pid=4226 ...
//     comm-tid   tgid    cpu  flags    time       event        payload
//
// Only the CPU, the timestamp, the event name and the payload are structurally needed, so the
// leading columns are matched loosely. Each event type is then read by its own grammar: ftrace
// payloads are event-specific, and comm names may contain spaces (`comm=shell svc 4391`), which
// rules out one generic `key=value` split.
//
// What is extracted is what a Perfetto ftrace capture can actually contain, mapped to the shapes
// this module can draw:
//
//   sched_switch            per-thread running spans, per-CPU lanes, runnable intervals
//   sched_waking/wakeup     where a runnable interval begins, and where a sleep ends
//   sched_blocked_reason    why a thread slept, attached to the sleep it caused
//   cpu_frequency           per-CPU frequency metric (step)
//   cpu_idle                per-CPU idle/busy state metric (step)
//   clk_set_rate            clock rate metric (step)
//   clk_enable/clk_disable  clock user count metric (step)
//   irq_handler_*           per-CPU interrupt lanes
//   softirq_*               per-CPU softirq lanes
//   workqueue_execute_*     work-item spans on the worker thread that ran them
//   task_newtask/rename     the comm a thread carried, which is the only name a raw systrace has
//   sched_process_exit      closes a sleep that never ended because the thread went away
//   tracing_mark_write      atrace begin/end, async begin/end and counter marks
//
// Event types that are not modelled are counted and named in a warning rather than silently
// dropped, so the panel says what it is not showing. What remains unmodelled by design is the
// scheduler's internal bookkeeping (`sched_pi_setprio`, `oom_score_adj_update`,
// `suspend_resume`): they change state that no lane in this module draws, so inventing a lane for
// them would add rows without adding information.
struct SystraceRecord
{
  int cpu = -1;
  int tid = -1;
  double timeSec = 0.0;
  QString event;
  QString payload;
};

// Parses one systrace line. Returns false for anything that is not an event: the file banner, the
// blank separators, and Perfetto's trailing statistics lines.
bool ParseSystraceRecord(const QString &line, SystraceRecord &rec)
{
  // The comm may itself contain '-', and the tgid column is '-----' for a kernel thread, so the tgid
  // is captured loosely rather than as a number.
  static const QRegularExpression pattern(QStringLiteral(
      "^(.*?)-(\\d+)\\s+\\(([^)]*)\\)\\s*\\[\\s*(\\d+)\\][^\\r\\n]*?\\s(\\d+\\.\\d+):\\s+"
      "([A-Za-z0-9_]+):\\s?(.*)$"));

  const QRegularExpressionMatch match = pattern.match(line);
  if(!match.hasMatch())
    return false;

  rec.tid = match.captured(2).toInt();
  rec.cpu = match.captured(4).toInt();
  rec.timeSec = match.captured(5).toDouble();
  rec.event = match.captured(6);
  rec.payload = match.captured(7);
  return true;
}

// Transient state for one Systrace text: everything that has to be closed again at the end, plus
// the metric series that are created the first time an event of their type is seen.
struct SystraceState
{
  struct Running
  {
    int tid = -1;
    QString comm;
    double beginSec = 0.0;
    bool valid = false;
  };

  // Per CPU: the thread currently on it.
  QHash<int, Running> running;
  // Per CPU: the span still open on that CPU's lane.
  QHash<int, TraceSpan> cpuOpen;
  // tid -> the time it became runnable and has not run since.
  QHash<int, double> runnableSince;
  // tid -> the time it stopped running because it blocked. The interval from here to the moment it
  // is woken is what a jank is usually made of, so it is drawn rather than left as a gap.
  QHash<int, double> sleepingSince;
  // tid -> the reason `sched_blocked_reason` gave for the sleep that is currently open. Consumed by
  // the span that closes it, so a stale reason cannot caption an unrelated sleep.
  QHash<int, QString> blockedReason;
  // Per CPU: open interrupt / softirq blocks. Both can nest, so they are stacks.
  QHash<int, QVector<TraceSpan>> irqStack;
  QHash<int, QVector<TraceSpan>> softirqStack;
  // Per tid: open workqueue executions. A worker can process work that queues more work, so this is
  // a stack per thread rather than a single open span.
  QHash<int, QVector<TraceSpan>> workStacks;
  // Clock name -> how many users currently hold it enabled. `clk_enable`/`clk_disable` nest.
  QHash<QString, int> clockEnableCount;
  // Per pid: the open atrace 'B' blocks, outermost last.
  QHash<int, QVector<TraceSpan>> atraceStacks;
  // "pid|name|cookie" -> the open async span.
  QHash<QString, TraceSpan> asyncOpen;
  // Series name -> index into builder.series.
  QHash<QString, int> seriesIndex;
  // tid -> index into builder.lanes, so a thread keeps one lane no matter which event type announced
  // it first. See SystraceLaneFor().
  QHash<int, int> laneByTid;
  bool lanesSeeded = false;
  int clockSeries = 0;
  double lastTimeSec = 0.0;
  bool any = false;
};

// Lanes are keyed by (pid, tid), but a scheduler slice only ever names a tid: the process a thread
// belongs to is learned from its atrace marks, which can appear after its first slice. Lanes in this
// text are therefore looked up by tid alone and adopt their owning process as soon as it is known,
// so one thread is one row rather than one row per way it was discovered.
RawLane &SystraceLaneFor(ChromeTraceBuilder &builder, SystraceState &state, int ownerPid, int tid)
{
  if(tid < 0)
    return LaneFor(builder, ownerPid, tid);

  if(!state.lanesSeeded)
  {
    for(int i = 0; i < builder.lanes.size(); i++)
    {
      if(builder.lanes[i].tid >= 0 && !state.laneByTid.contains(builder.lanes[i].tid))
        state.laneByTid.insert(builder.lanes[i].tid, i);
    }

    state.lanesSeeded = true;
  }

  const auto it = state.laneByTid.constFind(tid);
  if(it != state.laneByTid.constEnd())
  {
    RawLane &lane = builder.lanes[it.value()];

    // Adopt the owning process once it is known; a lane that already has a real pid keeps it.
    if(ownerPid >= 0 && (lane.pid < 0 || lane.pid == lane.tid))
      lane.pid = ownerPid;

    return lane;
  }

  // Until an owner is known the lane is captioned by its thread id, which is the only identity the
  // scheduler events gave it.
  RawLane &lane = LaneFor(builder, ownerPid >= 0 ? ownerPid : tid, tid);
  state.laneByTid.insert(tid, builder.lanes.size() - 1);
  return lane;
}

int SeriesIndexFor(ChromeTraceBuilder &builder, SystraceState &state, const QString &name,
                   const QString &unit, bool step, const QString &group, int pid)
{
  const auto it = state.seriesIndex.constFind(name);
  if(it != state.seriesIndex.constEnd())
    return it.value();

  TraceSeries s;
  s.name = name;
  s.unit = unit;
  s.step = step;
  s.group = group;
  s.pid = pid;

  const int index = builder.series.size();
  builder.series.push_back(s);
  state.seriesIndex.insert(name, index);

  return index;
}

void AddSystraceSample(ChromeTraceBuilder &builder, SystraceState &state, const QString &name,
                       const QString &unit, bool step, const QString &group, int pid,
                       double timeSec, double value)
{
  const int index = SeriesIndexFor(builder, state, name, unit, step, group, pid);

  TraceSample sample;
  sample.timeSec = timeSec;
  sample.value = value;
  builder.series[index].samples.push_back(sample);

  builder.stats.systraceCounterSamples++;
}

// The per-CPU lane shows which thread occupied the CPU, which is what a scheduler timeline is read
// for. It is kept separate from the per-thread lanes because the two answer different questions:
// "what was this CPU doing" versus "where did this thread run".
// Lane rows that belong to a CPU rather than to a thread (the scheduler row, the interrupt row, the
// softirq row) all grow on demand and are captioned by CPU number alone.
RawLane &AuxCpuLaneFor(QVector<RawLane> &lanes, int cpu)
{
  while(lanes.size() <= cpu)
  {
    RawLane lane;
    lane.pid = -1;
    lane.tid = -1;
    lanes.push_back(lane);
  }

  return lanes[cpu];
}

RawLane &CpuLaneFor(ChromeTraceBuilder &builder, int cpu)
{
  return AuxCpuLaneFor(builder.cpuLanes, cpu);
}

void CloseCpuSpan(ChromeTraceBuilder &builder, SystraceState &state, int cpu, double timeSec)
{
  const auto open = state.cpuOpen.find(cpu);
  if(open == state.cpuOpen.end())
    return;

  TraceSpan span = open.value();
  span.endSec = timeSec;
  state.cpuOpen.erase(open);

  if(span.endSec < span.beginSec)
  {
    builder.stats.reversedSpans++;
    return;
  }

  CpuLaneFor(builder, cpu).spans.push_back(span);
  builder.stats.spans++;
  builder.stats.systraceSpans++;
}

// Closes the interval a thread spent blocked. The state it was switched out in and the reason the
// kernel gave are attached to the span instead of being dropped: waiting on storage and waiting on a
// lock look identical on a timeline otherwise, and they point at completely different fixes.
void CloseSleepingSpan(ChromeTraceBuilder &builder, SystraceState &state, int tid, double timeSec)
{
  const auto sleep = state.sleepingSince.find(tid);
  if(sleep == state.sleepingSince.end())
    return;

  const double beginSec = sleep.value();
  state.sleepingSince.erase(sleep);

  QString reason;
  const auto blocked = state.blockedReason.find(tid);
  if(blocked != state.blockedReason.end())
  {
    reason = blocked.value();
    state.blockedReason.erase(blocked);
  }

  if(timeSec < beginSec)
  {
    builder.stats.reversedSpans++;
    return;
  }

  TraceSpan span;
  span.beginSec = beginSec;
  span.endSec = timeSec;
  span.name = QStringLiteral("Sleeping");
  span.category = QStringLiteral("sched-sleeping");
  span.fields << QStringLiteral("tid = %1").arg(tid)
              << QStringLiteral("duration = %1 ms").arg((timeSec - beginSec) * 1000.0, 0, 'f', 3);

  if(!reason.isEmpty())
    span.fields << QStringLiteral("blocked_reason = %1").arg(reason);

  SystraceLaneFor(builder, state, -1, tid).spans.push_back(span);
  builder.stats.spans++;
  builder.stats.systraceSpans++;
}

void ConsumeSchedSwitch(const SystraceRecord &rec, ChromeTraceBuilder &builder, SystraceState &state)
{
  static const QRegularExpression pattern(QStringLiteral(
      "^prev_comm=(.*?) prev_pid=(-?\\d+) prev_prio=-?\\d+ prev_state=(\\S+) ==> "
      "next_comm=(.*?) next_pid=(-?\\d+)"));

  const QRegularExpressionMatch match = pattern.match(rec.payload);
  if(!match.hasMatch())
  {
    builder.stats.malformed++;
    return;
  }

  const QString prevComm = match.captured(1);
  const int prevTid = match.captured(2).toInt();
  const QString prevState = match.captured(3);
  const QString nextComm = match.captured(4);
  const int nextTid = match.captured(5).toInt();

  // Every sched_switch names both threads it moved, so the comm of a thread is known even in a trace
  // that declares no `thread_name` metadata at all.
  if(prevTid >= 0)
    builder.commByTid.insert(prevTid, prevComm);
  if(nextTid >= 0)
    builder.commByTid.insert(nextTid, nextComm);

  // The thread that was on this CPU stops running here.
  const auto current = state.running.constFind(rec.cpu);
  if(current != state.running.constEnd() && current->valid)
  {
    TraceSpan span;
    span.beginSec = current->beginSec;
    span.endSec = rec.timeSec;
    span.name = current->comm;
    span.category = QStringLiteral("sched");
    span.fields << QStringLiteral("cpu = %1").arg(rec.cpu)
                << QStringLiteral("tid = %1").arg(current->tid);

    if(span.endSec < span.beginSec)
    {
      builder.stats.reversedSpans++;
    }
    else
    {
      SystraceLaneFor(builder, state, -1, current->tid).spans.push_back(span);
      builder.stats.spans++;
      builder.stats.systraceSpans++;
    }
  }

  // 'R' means the thread it replaced was still ready to run: it leaves the CPU but stays runnable,
  // which is the state that explains a jank. Any other state means it blocked, so a runnable
  // interval that was still open has to be discarded rather than extended over a sleep, and the
  // sleep itself starts here.
  if(prevState.startsWith(QLatin1Char('R')))
  {
    state.runnableSince.insert(prevTid, rec.timeSec);
  }
  else
  {
    state.runnableSince.remove(prevTid);

    // Only the first switch-out opens a sleep; a thread already asleep is not re-timed by a later
    // record, which would shorten the interval it is being judged on.
    if(prevTid >= 0 && !state.sleepingSince.contains(prevTid))
      state.sleepingSince.insert(prevTid, rec.timeSec);
  }

  CloseCpuSpan(builder, state, rec.cpu, rec.timeSec);

  // A thread can start running without a matching wakeup in the trace (the wakeup predates the
  // trace window, or it came from an interrupt that was not recorded), so a sleep left open by the
  // incoming thread is closed here rather than at the end of the trace.
  CloseSleepingSpan(builder, state, nextTid, rec.timeSec);

  // The thread that comes in stops being runnable at exactly this point.
  const auto runnable = state.runnableSince.find(nextTid);
  if(runnable != state.runnableSince.end())
  {
    if(rec.timeSec > runnable.value())
    {
      TraceSpan span;
      span.beginSec = runnable.value();
      span.endSec = rec.timeSec;
      span.name = QStringLiteral("Runnable");
      span.category = QStringLiteral("sched-runnable");
      span.fields << QStringLiteral("tid = %1").arg(nextTid);

      SystraceLaneFor(builder, state, -1, nextTid).spans.push_back(span);
      builder.stats.spans++;
      builder.stats.systraceSpans++;
    }

    state.runnableSince.erase(runnable);
  }

  SystraceState::Running next;
  next.tid = nextTid;
  next.comm = nextComm;
  next.beginSec = rec.timeSec;
  next.valid = true;
  state.running.insert(rec.cpu, next);

  TraceSpan cpuSpan;
  cpuSpan.beginSec = rec.timeSec;
  cpuSpan.endSec = rec.timeSec;
  cpuSpan.name = nextComm;
  cpuSpan.category = QStringLiteral("sched");
  cpuSpan.fields << QStringLiteral("tid = %1").arg(nextTid);
  state.cpuOpen.insert(rec.cpu, cpuSpan);
}

void ConsumeWakeup(const SystraceRecord &rec, ChromeTraceBuilder &builder, SystraceState &state)
{
  static const QRegularExpression pattern(QStringLiteral("^comm=.*? pid=(-?\\d+) prio=-?\\d+"));

  const QRegularExpressionMatch match = pattern.match(rec.payload);
  if(!match.hasMatch())
    return;

  // `sched_waking` and `sched_wakeup` describe the same transition, the second once the target CPU
  // has been chosen. Whichever arrives first owns the end of the sleep and the start of the runnable
  // interval; the duplicate is harmless because both are guarded by `contains`.
  const int tid = match.captured(1).toInt();

  CloseSleepingSpan(builder, state, tid, rec.timeSec);

  if(!state.runnableSince.contains(tid))
    state.runnableSince.insert(tid, rec.timeSec);
}

// `sched_blocked_reason: pid=4079 iowait=0 caller=1`. The payload is kept verbatim: `caller` is a
// kernel call-site id that the trace does not name, so decoding it into a label would be a guess.
void ConsumeBlockedReason(const SystraceRecord &rec, SystraceState &state)
{
  static const QRegularExpression pattern(QStringLiteral("^pid=(-?[0-9]+) +(.*)$"));

  const QRegularExpressionMatch match = pattern.match(rec.payload);
  if(!match.hasMatch())
    return;

  const int payloadPid = match.captured(1).toInt();
  state.blockedReason.insert(payloadPid >= 0 ? payloadPid : rec.tid,
                            match.captured(2).trimmed());
}

// `task_newtask: pid=4389 comm=UnityGfxDeviceW clone_flags=...` and
// `task_rename: pid=4389 oldcomm=UnityGfxDeviceW newcomm=HWC release oom_score_adj=0`. The comm can
// contain spaces, so it is captured up to the field that follows it rather than as one token.
void ConsumeTaskName(const SystraceRecord &rec, ChromeTraceBuilder &builder)
{
  if(rec.event == QLatin1String("task_newtask"))
  {
    static const QRegularExpression pattern(
        QStringLiteral("^pid=(-?[0-9]+) comm=(.*?) clone_flags="));
    const QRegularExpressionMatch match = pattern.match(rec.payload);
    if(!match.hasMatch())
    {
      builder.stats.malformed++;
      return;
    }

    builder.commByTid.insert(match.captured(1).toInt(), match.captured(2));
    return;
  }

  static const QRegularExpression pattern(
      QStringLiteral("^pid=(-?[0-9]+) oldcomm=(.*?) newcomm=(.*?)(?: oom_score_adj=|$)"));
  const QRegularExpressionMatch match = pattern.match(rec.payload);
  if(!match.hasMatch())
  {
    builder.stats.malformed++;
    return;
  }

  builder.commByTid.insert(match.captured(1).toInt(), match.captured(3));
}

// A thread that exits while asleep never produces the wakeup that would close its sleep, so the exit
// is the end of it. Only the open sleep is touched: the running span was already closed by the
// switch-out that preceded the exit.
void ConsumeProcessExit(const SystraceRecord &rec, ChromeTraceBuilder &builder, SystraceState &state)
{
  static const QRegularExpression pattern(QStringLiteral("^comm=(.*?) pid=(-?[0-9]+) prio="));

  const QRegularExpressionMatch match = pattern.match(rec.payload);
  if(!match.hasMatch())
  {
    builder.stats.malformed++;
    return;
  }

  CloseSleepingSpan(builder, state, match.captured(2).toInt(), rec.timeSec);
}

void ConsumeCpuIdle(const SystraceRecord &rec, ChromeTraceBuilder &builder, SystraceState &state)
{
  static const QRegularExpression pattern(QStringLiteral("^state=(\\d+) cpu_id=(\\d+)"));

  const QRegularExpressionMatch match = pattern.match(rec.payload);
  if(!match.hasMatch())
  {
    builder.stats.malformed++;
    return;
  }

  const qulonglong idleState = match.captured(1).toULongLong();
  const int cpu = match.captured(2).toInt();

  // `state` is the idle state being entered; 0xFFFFFFFF marks leaving idle. The series is a level,
  // so it is drawn as a step: interpolating it would draw partially-idle CPUs that never existed.
  AddSystraceSample(builder, state, QObject::tr("CPU %1 Idle").arg(cpu), QString(), true,
                    QObject::tr("CPU Idle"), -1, rec.timeSec,
                    idleState == 0xFFFFFFFFull ? 0.0 : 1.0);
}

void ConsumeCpuFrequency(const SystraceRecord &rec, ChromeTraceBuilder &builder, SystraceState &state)
{
  static const QRegularExpression pattern(QStringLiteral("^state=(\\d+) cpu_id=(\\d+)"));

  const QRegularExpressionMatch match = pattern.match(rec.payload);
  if(!match.hasMatch())
  {
    builder.stats.malformed++;
    return;
  }

  // `state` is in kHz. MHz keeps the label short and the value readable on a lane that is only a
  // few pixels tall.
  const double mhz = match.captured(1).toDouble() / 1000.0;
  const int cpu = match.captured(2).toInt();

  AddSystraceSample(builder, state, QObject::tr("CPU %1 Frequency").arg(cpu),
                    QStringLiteral("MHz"), true, QObject::tr("CPU Frequency"), -1, rec.timeSec, mhz);
}

void ConsumeClockSetRate(const SystraceRecord &rec, ChromeTraceBuilder &builder, SystraceState &state)
{
  static const QRegularExpression pattern(QStringLiteral("^(\\S+) (\\d+)$"));

  const QRegularExpressionMatch match = pattern.match(rec.payload);
  if(!match.hasMatch())
  {
    builder.stats.malformed++;
    return;
  }

  const QString clock = match.captured(1);
  const QString name = QObject::tr("Clock %1").arg(clock);

  if(!state.seriesIndex.contains(name))
  {
    if(state.clockSeries >= MaxClockSeries)
    {
      builder.stats.droppedClockSeries++;
      return;
    }

    state.clockSeries++;
  }

  AddSystraceSample(builder, state, name, QStringLiteral("MHz"), true, QObject::tr("Clocks"),
                    -1, rec.timeSec, match.captured(2).toDouble() / 1.0e6);
}

// `clk_enable: name=gcc_ufs_phy_axi_clk_src` / `clk_disable: name=...`. A rate series says how fast a
// clock runs; this says whether anyone was using it, which is what explains a long clock-gated stall
// on a peripheral (storage, display) that never shows up as CPU work.
void ConsumeClockEnable(const SystraceRecord &rec, ChromeTraceBuilder &builder, SystraceState &state,
                        bool enable)
{
  static const QRegularExpression pattern(QStringLiteral("^name=(.*)$"));

  const QRegularExpressionMatch match = pattern.match(rec.payload);
  if(!match.hasMatch())
  {
    builder.stats.malformed++;
    return;
  }

  const QString clock = match.captured(1).trimmed();
  // Enable/disable nest, so the value is a user count and only a count of zero means "gated off".
  int users = state.clockEnableCount.value(clock) + (enable ? 1 : -1);
  if(users < 0)
    users = 0;
  state.clockEnableCount.insert(clock, users);

  const QString name = QObject::tr("Clock %1 users").arg(clock);
  if(!state.seriesIndex.contains(name))
  {
    if(state.clockSeries >= MaxClockSeries)
    {
      builder.stats.droppedClockSeries++;
      return;
    }

    state.clockSeries++;
  }

  AddSystraceSample(builder, state, name, QString(), true, QObject::tr("Clocks"), -1, rec.timeSec,
                    users);
}

// Interrupt context is not a thread: `irq_handler_entry` runs on whichever thread the CPU happened to
// be running, so the span belongs to the CPU's interrupt row. Handlers can nest (a timer IRQ raising
// a softirq), hence one stack per CPU.
void ConsumeIrqEntry(const SystraceRecord &rec, SystraceState &state)
{
  static const QRegularExpression pattern(QStringLiteral("^irq=(-?[0-9]+) name=(.*)$"));

  const QRegularExpressionMatch match = pattern.match(rec.payload);
  if(!match.hasMatch())
    return;

  TraceSpan span;
  span.beginSec = rec.timeSec;
  span.endSec = rec.timeSec;
  span.name = match.captured(2).trimmed();
  span.category = QStringLiteral("irq");
  span.fields << QStringLiteral("irq = %1").arg(match.captured(1))
              << QStringLiteral("cpu = %1").arg(rec.cpu)
              << QStringLiteral("preempted tid = %1").arg(rec.tid);

  state.irqStack[rec.cpu].push_back(span);
}

void ConsumeIrqExit(const SystraceRecord &rec, ChromeTraceBuilder &builder, SystraceState &state)
{
  const auto stack = state.irqStack.find(rec.cpu);
  if(stack == state.irqStack.end() || stack->isEmpty())
  {
    builder.stats.unterminatedSpans++;
    return;
  }

  TraceSpan span = stack->takeLast();
  span.endSec = rec.timeSec;

  if(span.endSec < span.beginSec)
  {
    builder.stats.reversedSpans++;
    return;
  }

  AuxCpuLaneFor(builder.irqLanes, rec.cpu).spans.push_back(span);
  builder.stats.spans++;
  builder.stats.systraceSpans++;
}

// The kernel numbers softirq vectors and does not name them in the event, so the vector is mapped to
// the name the kernel source gives it. An `action` in the payload wins when the kernel emitted one.
QString SoftirqName(int vec, const QString &action)
{
  if(!action.isEmpty())
    return action;

  static const char *const names[] = {"HI",     "TIMER",  "NET_TX", "NET_RX", "BLOCK",
                                      "IRQ_POLL", "TASKLET", "SCHED", "HRTIMER", "RCU"};

  if(vec >= 0 && vec < (int)(sizeof(names) / sizeof(names[0])))
    return QStringLiteral("softirq %1").arg(QLatin1String(names[vec]));

  return QStringLiteral("softirq vec %1").arg(vec);
}

void ConsumeSoftirqEntry(const SystraceRecord &rec, SystraceState &state)
{
  static const QRegularExpression vecPattern(QStringLiteral("^vec=(-?[0-9]+)"));
  static const QRegularExpression actionPattern(QStringLiteral("action=([^]]*)"));

  const QRegularExpressionMatch vec = vecPattern.match(rec.payload);
  if(!vec.hasMatch())
    return;

  const QRegularExpressionMatch action = actionPattern.match(rec.payload);
  const int vector = vec.captured(1).toInt();

  TraceSpan span;
  span.beginSec = rec.timeSec;
  span.endSec = rec.timeSec;
  span.name = SoftirqName(vector, action.hasMatch() ? action.captured(1).trimmed() : QString());
  span.category = QStringLiteral("softirq");
  span.fields << QStringLiteral("vec = %1").arg(vector)
              << QStringLiteral("cpu = %1").arg(rec.cpu)
              << QStringLiteral("preempted tid = %1").arg(rec.tid);

  state.softirqStack[rec.cpu].push_back(span);
}

void ConsumeSoftirqExit(const SystraceRecord &rec, ChromeTraceBuilder &builder, SystraceState &state)
{
  const auto stack = state.softirqStack.find(rec.cpu);
  if(stack == state.softirqStack.end() || stack->isEmpty())
  {
    builder.stats.unterminatedSpans++;
    return;
  }

  TraceSpan span = stack->takeLast();
  span.endSec = rec.timeSec;

  if(span.endSec < span.beginSec)
  {
    builder.stats.reversedSpans++;
    return;
  }

  AuxCpuLaneFor(builder.softirqLanes, rec.cpu).spans.push_back(span);
  builder.stats.spans++;
  builder.stats.systraceSpans++;
}

// Workqueue executions run on a worker thread, so they belong on that thread's lane. The symbol tells
// the reader what the worker did; the offset is kept as a field so two calls to the same function stay
// distinguishable without making the slice label unreadable.
void ConsumeWorkqueueStart(const SystraceRecord &rec, SystraceState &state)
{
  static const QRegularExpression pattern(QStringLiteral("function=(.*)$"));

  const QRegularExpressionMatch match = pattern.match(rec.payload);
  if(!match.hasMatch())
    return;

  QString function = match.captured(1).trimmed();
  QString offset;
  const int plus = function.lastIndexOf(QLatin1Char('+'));
  if(plus > 0)
  {
    offset = function.mid(plus + 1);
    function = function.left(plus);
  }

  TraceSpan span;
  span.beginSec = rec.timeSec;
  span.endSec = rec.timeSec;
  span.name = function;
  span.category = QStringLiteral("workqueue");
  span.fields << QStringLiteral("tid = %1").arg(rec.tid);
  if(!offset.isEmpty())
    span.fields << QStringLiteral("offset = %1").arg(offset);

  state.workStacks[rec.tid].push_back(span);
}

void ConsumeWorkqueueEnd(const SystraceRecord &rec, ChromeTraceBuilder &builder, SystraceState &state)
{
  const auto stack = state.workStacks.find(rec.tid);
  if(stack == state.workStacks.end() || stack->isEmpty())
  {
    builder.stats.unterminatedSpans++;
    return;
  }

  TraceSpan span = stack->takeLast();
  span.endSec = rec.timeSec;

  if(span.endSec < span.beginSec)
  {
    builder.stats.reversedSpans++;
    return;
  }

  SystraceLaneFor(builder, state, -1, rec.tid).spans.push_back(span);
  builder.stats.spans++;
  builder.stats.systraceSpans++;
}

// atrace marks arrive through `tracing_mark_write` with their own field grammar:
//
//   B|pid|name            begin, closed by the next E|pid on that pid
//   E|pid                 end
//   S|pid|name|cookie     async begin, closed by F|pid|name|cookie
//   F|pid|name|cookie     async end
//   C|pid|name|value      counter sample
//
// The name is free text (surface names, "Choreographer#doFrame 174031") so only the leading two
// fields and, for counters and async marks, the trailing field are taken structurally.
void ConsumeAtrace(const SystraceRecord &rec, ChromeTraceBuilder &builder, SystraceState &state)
{
  const QString &payload = rec.payload;

  // Perfetto's own clock-sync marker travels through the same event but has no pipe-separated fields:
  // it records the offset between the trace clock and the capture clock and draws nothing, so it is
  // not a malformed atrace mark.
  if(payload.startsWith(QLatin1String("trace_event_clock_sync")))
    return;

  builder.stats.atraceMarks++;

  const int kindEnd = payload.indexOf(QLatin1Char('|'));
  if(kindEnd <= 0)
  {
    builder.stats.malformed++;
    return;
  }

  const QString kind = payload.left(kindEnd);
  const int pidEnd = payload.indexOf(QLatin1Char('|'), kindEnd + 1);
  const QString pidText =
      pidEnd < 0 ? payload.mid(kindEnd + 1) : payload.mid(kindEnd + 1, pidEnd - kindEnd - 1);
  const int pid = pidText.toInt();
  const QString rest = pidEnd < 0 ? QString() : payload.mid(pidEnd + 1);

  if(kind == QLatin1String("B"))
  {
    TraceSpan span;
    span.beginSec = rec.timeSec;
    span.endSec = rec.timeSec;
    span.name = rest;
    span.category = QStringLiteral("atrace");
    state.atraceStacks[pid].push_back(span);
    return;
  }

  if(kind == QLatin1String("E"))
  {
    QVector<TraceSpan> &stack = state.atraceStacks[pid];
    if(stack.isEmpty())
    {
      builder.stats.unterminatedSpans++;
      return;
    }

    TraceSpan span = stack.takeLast();
    span.endSec = rec.timeSec;

    if(span.endSec < span.beginSec)
    {
      builder.stats.reversedSpans++;
      return;
    }

    RawLane &lane = SystraceLaneFor(builder, state, pid, rec.tid >= 0 ? rec.tid : pid);
    lane.atrace = true;
    lane.spans.push_back(span);
    builder.stats.spans++;
    builder.stats.systraceSpans++;
    return;
  }

  if(kind == QLatin1String("S") || kind == QLatin1String("F"))
  {
    const int cookieStart = rest.lastIndexOf(QLatin1Char('|'));
    if(cookieStart <= 0)
    {
      builder.stats.malformed++;
      return;
    }

    const QString name = rest.left(cookieStart);
    const QString key = QStringLiteral("%1|%2|%3").arg(pid).arg(name, rest.mid(cookieStart + 1));

    if(kind == QLatin1String("S"))
    {
      TraceSpan span;
      span.beginSec = rec.timeSec;
      span.endSec = rec.timeSec;
      span.name = name;
      span.category = QStringLiteral("atrace-async");
      state.asyncOpen.insert(key, span);
      return;
    }

    const auto open = state.asyncOpen.find(key);
    if(open == state.asyncOpen.end())
    {
      // An async end whose begin was never captured: there is no interval to close.
      builder.stats.unterminatedSpans++;
      return;
    }

    TraceSpan span = open.value();
    span.endSec = rec.timeSec;
    state.asyncOpen.erase(open);

    if(span.endSec < span.beginSec)
    {
      builder.stats.reversedSpans++;
      return;
    }

    RawLane &lane = SystraceLaneFor(builder, state, pid, rec.tid >= 0 ? rec.tid : pid);
    lane.atrace = true;
    lane.spans.push_back(span);
    builder.stats.spans++;
    builder.stats.systraceSpans++;
    return;
  }

  if(kind == QLatin1String("C"))
  {
    const int valueStart = rest.lastIndexOf(QLatin1Char('|'));
    if(valueStart <= 0)
    {
      builder.stats.malformed++;
      return;
    }

    bool ok = false;
    const double value = rest.mid(valueStart + 1).toDouble(&ok);
    if(!ok)
    {
      builder.stats.malformed++;
      return;
    }

    const QString name = rest.left(valueStart);

    // The pid is recorded on the series so its metric lane lands in the owning process's group.
    AddSystraceSample(builder, state, name, QString(), false, builder.processNames.value(pid), pid,
                      rec.timeSec, value);
    return;
  }

  // 'N' (track name), 'I' (instant), 'A' (legacy async) and vendor-specific marks have no interval
  // to draw, so they are counted rather than coerced into a shape they do not have.
  builder.stats.systraceUnmodelled++;
  builder.unmodelledEvents.insert(QStringLiteral("tracing_mark_write %1").arg(kind), 1);
}

// A handful of the kernel's ftrace probes are annotations rather than intervals: they report that
// something happened at one instant and carry no end, so there is nothing to draw on a lane. Naming
// them here keeps the "no lane or metric" warning about events the timeline genuinely failed to
// understand instead of nagging about ones it deliberately leaves to a future instant-marker track.
static bool IsPointOnlyEvent(const QString &name)
{
  static const char *const names[] = {
      "binder_set_priority",  "binder_transaction",     "binder_transaction_alloc_buf",
      "binder_transaction_received", "oom_score_adj_update", "sched_pi_setprio",
      "softirq_raise",        "ipi_raise",              "workqueue_queue_work",
      "workqueue_activate_work",
  };

  for(const char *candidate : names)
  {
    if(name == QLatin1String(candidate))
      return true;
  }

  return false;
}

// Reads the whole ftrace text and closes everything it opened. Returns true when at least one
// record was recognised.
bool ConsumeSystemTraceEvents(const QString &ftrace, ChromeTraceBuilder &builder)
{
  SystraceState state;

  const QStringList lines = ftrace.split(QLatin1Char('\n'));
  SystraceRecord rec;

  for(const QString &line : lines)
  {
    if(line.isEmpty())
      continue;

    if(!ParseSystraceRecord(line, rec))
      continue;

    state.any = true;
    // The end of the trace is the latest timestamp, not the last one written: Perfetto appends its own
    // clock-sync marker with a zero timestamp, and taking that as the end would close every interval
    // still open at the end of the file at time zero.
    state.lastTimeSec = qMax(state.lastTimeSec, rec.timeSec);
    builder.stats.systraceEvents++;

    if(rec.event == QLatin1String("sched_switch"))
    {
      ConsumeSchedSwitch(rec, builder, state);
    }
    else if(rec.event == QLatin1String("tracing_mark_write"))
    {
      ConsumeAtrace(rec, builder, state);
    }
    else if(rec.event == QLatin1String("cpu_idle"))
    {
      ConsumeCpuIdle(rec, builder, state);
    }
    else if(rec.event == QLatin1String("sched_waking") || rec.event == QLatin1String("sched_wakeup"))
    {
      ConsumeWakeup(rec, builder, state);
    }
    else if(rec.event == QLatin1String("sched_blocked_reason"))
    {
      ConsumeBlockedReason(rec, state);
    }
    else if(rec.event == QLatin1String("sched_process_exit"))
    {
      ConsumeProcessExit(rec, builder, state);
    }
    else if(rec.event == QLatin1String("task_newtask") || rec.event == QLatin1String("task_rename"))
    {
      ConsumeTaskName(rec, builder);
    }
    else if(rec.event == QLatin1String("clk_enable"))
    {
      ConsumeClockEnable(rec, builder, state, true);
    }
    else if(rec.event == QLatin1String("clk_disable"))
    {
      ConsumeClockEnable(rec, builder, state, false);
    }
    else if(rec.event == QLatin1String("irq_handler_entry"))
    {
      ConsumeIrqEntry(rec, state);
    }
    else if(rec.event == QLatin1String("irq_handler_exit"))
    {
      ConsumeIrqExit(rec, builder, state);
    }
    else if(rec.event == QLatin1String("softirq_entry"))
    {
      ConsumeSoftirqEntry(rec, state);
    }
    else if(rec.event == QLatin1String("softirq_exit"))
    {
      ConsumeSoftirqExit(rec, builder, state);
    }
    else if(rec.event == QLatin1String("workqueue_execute_start"))
    {
      ConsumeWorkqueueStart(rec, state);
    }
    else if(rec.event == QLatin1String("workqueue_execute_end"))
    {
      ConsumeWorkqueueEnd(rec, builder, state);
    }
    else if(rec.event == QLatin1String("cpu_frequency"))
    {
      ConsumeCpuFrequency(rec, builder, state);
    }
    else if(rec.event == QLatin1String("clk_set_rate"))
    {
      ConsumeClockSetRate(rec, builder, state);
    }
    else
    {
      builder.stats.systraceUnmodelled++;

      // Point events are understood, they just have no interval to occupy; only report the ones this
      // build has no idea about.
      if(!IsPointOnlyEvent(rec.event))
        builder.unmodelledEvents.insert(rec.event, 1);
    }
  }

  if(!state.any)
    return false;

  const double endSec = state.lastTimeSec;

  // Everything still open is closed at the last timestamp the trace recorded. Dropping it would
  // hide work that really happened, while inventing an end beyond the trace would draw off the
  // timeline, so the true end is reported as unknown through the warning the caller emits.
  for(auto it = state.running.begin(); it != state.running.end(); ++it)
  {
    if(!it->valid)
      continue;

    TraceSpan span;
    span.beginSec = it->beginSec;
    span.endSec = endSec;
    span.name = it->comm;
    span.category = QStringLiteral("sched");
    span.fields << QStringLiteral("cpu = %1").arg(it.key())
                << QStringLiteral("tid = %1").arg(it->tid);

    if(span.endSec >= span.beginSec)
    {
      SystraceLaneFor(builder, state, -1, it->tid).spans.push_back(span);
      builder.stats.spans++;
      builder.stats.systraceSpans++;
    }
  }

  QVector<int> cpus;
  for(auto it = state.cpuOpen.constBegin(); it != state.cpuOpen.constEnd(); ++it)
    cpus.push_back(it.key());
  for(int cpu : cpus)
    CloseCpuSpan(builder, state, cpu, endSec);

  for(auto it = state.atraceStacks.begin(); it != state.atraceStacks.end(); ++it)
  {
    RawLane &lane = SystraceLaneFor(builder, state, -1, it.key());
    lane.atrace = true;
    for(TraceSpan &span : it.value())
    {
      span.endSec = endSec;
      lane.spans.push_back(span);
      builder.stats.spans++;
      builder.stats.systraceSpans++;
    }
  }

  for(auto it = state.asyncOpen.begin(); it != state.asyncOpen.end(); ++it)
  {
    const int pid = it.key().section(QLatin1Char('|'), 0, 0).toInt();
    TraceSpan span = it.value();
    span.endSec = endSec;

    RawLane &lane = SystraceLaneFor(builder, state, pid, pid);
    lane.atrace = true;
    lane.spans.push_back(span);
    builder.stats.spans++;
    builder.stats.systraceSpans++;
  }

  // A thread still asleep when the trace stops sleeping was not woken within the window, so its
  // sleep is closed at the last timestamp like every other open interval.
  QVector<int> sleepers;
  for(auto it = state.sleepingSince.constBegin(); it != state.sleepingSince.constEnd(); ++it)
    sleepers.push_back(it.key());
  for(int tid : sleepers)
    CloseSleepingSpan(builder, state, tid, endSec);

  // Interrupt context does not always unwind before the trace ends either - a handler cut short by
  // the end of the buffer is real, and dropping it would leave the CPU row looking idle.
  for(auto it = state.irqStack.begin(); it != state.irqStack.end(); ++it)
  {
    for(TraceSpan &span : it.value())
    {
      span.endSec = endSec;
      if(span.endSec >= span.beginSec)
      {
        AuxCpuLaneFor(builder.irqLanes, it.key()).spans.push_back(span);
        builder.stats.spans++;
        builder.stats.systraceSpans++;
      }
    }
  }

  for(auto it = state.softirqStack.begin(); it != state.softirqStack.end(); ++it)
  {
    for(TraceSpan &span : it.value())
    {
      span.endSec = endSec;
      if(span.endSec >= span.beginSec)
      {
        AuxCpuLaneFor(builder.softirqLanes, it.key()).spans.push_back(span);
        builder.stats.spans++;
        builder.stats.systraceSpans++;
      }
    }
  }

  for(auto it = state.workStacks.begin(); it != state.workStacks.end(); ++it)
  {
    for(TraceSpan &span : it.value())
    {
      span.endSec = endSec;
      if(span.endSec >= span.beginSec)
      {
        SystraceLaneFor(builder, state, -1, it.key()).spans.push_back(span);
        builder.stats.spans++;
        builder.stats.systraceSpans++;
      }
    }
  }

  return true;
}
} // namespace

// Android's ftrace carries no notion of a captured frame, and `traceconv` emits no frame events, so
// a boundary has to be derived from a marker that is actually present. The preference order follows
// how directly each marker bounds one frame of the app under capture:
//
//   1. the app's frame callback, `Choreographer#doFrame <id>`, which fires once per frame it draws
//   2. Unity's `Director.ProcessFrame`, which is the frame the engine itself is executing when the
//      app drives its own loop instead of waiting on vsync
//   3. SurfaceFlinger's `prepareFrame`, which starts one composition
//   4. SurfaceFlinger's `handlePageFlip`, which bounds what actually reached the display
//
// The numbers assigned this way are 1-based ordinals within the trace, not ids the source declared.
// They are tagged with a `source` so correlation knows it may not match them against capture frame
// numbers.
void DeriveFrameBoundaries(SystemTrace &trace)
{
  static const char *const Rules[] = {
      "Choreographer#doFrame",
      "Director.ProcessFrame",
      "prepareFrame",
      "handlePageFlip",
  };

  for(const char *rule : Rules)
  {
    const QString prefix = QLatin1String(rule);
    QVector<double> marks;

    for(const TraceTrack &track : trace.tracks)
    {
      for(const TraceSpan &span : track.spans)
      {
        if(span.name.startsWith(prefix))
          marks.push_back(span.beginSec);
      }
    }

    if(marks.isEmpty())
      continue;

    std::sort(marks.begin(), marks.end());

    trace.frameBoundaries.clear();

    for(double mark : marks)
    {
      // Two markers describing the same boundary would otherwise become two frames that no
      // capture can match, so anything closer than half a millisecond is collapsed.
      if(!trace.frameBoundaries.isEmpty() &&
         mark - trace.frameBoundaries.last().timeSec < 0.0005)
        continue;

      TraceFrameBoundary boundary;
      boundary.frameNumber = (uint32_t)trace.frameBoundaries.size() + 1;
      boundary.timeSec = mark;
      boundary.source = QObject::tr("%1 marker").arg(prefix);
      trace.frameBoundaries.push_back(boundary);
    }

    return;
  }
}

// The parts of an import that turn whatever the builder accumulated into a SystemTrace. Both entry
// points - a Chrome trace document and a raw systrace text file - produce the same builder, so the
// lanes, frame boundaries and warnings are assembled in exactly one place.
SystemTrace BuildTraceFromBuilder(ChromeTraceBuilder &builder, const QString &path,
                                 const QString &formatName, bool systraceUsed,
                                 int systraceEventCount, bool sawEventArray)
{
  SystemTrace trace;
  trace.source = path;
  trace.formatName = formatName;
  // Timestamps are device-time seconds; there is no shared clock with the capture, so correlation
  // has to derive an offset from frame boundaries.
  trace.timeUnit = QStringLiteral("s");

  const ChromeEventStats &stats = builder.stats;

  if(!sawEventArray && stats.events == 0)
    trace.warnings << QObject::tr(
        "Trace has no top-level event array matching any supported shape; the file may not be a "
        "trace export at all");


  trace.series = builder.series;
  trace.frameBoundaries = builder.frames;

  for(TraceSeries &s : trace.series)
    s.Sort();

  std::sort(trace.frameBoundaries.begin(), trace.frameBoundaries.end(),
            [](const TraceFrameBoundary &a, const TraceFrameBoundary &b) {
              return a.timeSec < b.timeSec;
            });

  // --- Lane construction ------------------------------------------------------------------------
  //
  // Lanes with spans carry the real timeline. A span open at end-of-file ('B' with no 'E') is
  // retained and closed at the last observed timestamp: dropping it would hide work that really
  // happened, while inventing an end time beyond the trace would draw a box off the timeline.
  double lastTs = 0.0;
  bool anyTs = false;
  for(const RawLane &lane : builder.lanes)
  {
    for(const TraceSpan &span : lane.spans)
    {
      if(!anyTs || span.endSec > lastTs)
        lastTs = span.endSec;
      anyTs = true;
    }
  }

  int closedUnterminated = 0;
  for(auto it = builder.openSpans.begin(); it != builder.openSpans.end(); ++it)
  {
    if(it.value().isEmpty())
      continue;

    // The lane is located through the same key it was opened under.
    const quint32 pidBits = (quint32)(it.key() >> 32);
    const quint32 tidBits = (quint32)(it.key() & 0xFFFFFFFFull);
    const int pid = pidBits == 0xFFFFFFFFu ? -1 : (int)pidBits;
    const int tid = tidBits == 0xFFFFFFFFu ? -1 : (int)tidBits;

    RawLane &lane = LaneFor(builder, pid, tid);
    for(TraceSpan &span : it.value())
    {
      span.endSec = anyTs ? lastTs : span.beginSec;
      lane.spans.push_back(span);
      closedUnterminated++;
    }
  }

  // Counter lanes come first and are ordered by series, so the metrics that explain GPU-bound
  // frames stay at the top of the view the way SDP presents them. System-wide metrics (per-CPU
  // frequency, clocks) are ordered before per-process ones so their groups stay contiguous.
  QVector<int> seriesOrder;
  for(int i = 0; i < trace.series.size(); i++)
    seriesOrder.push_back(i);

  std::sort(seriesOrder.begin(), seriesOrder.end(), [&](int a, int b) {
    const TraceSeries &sa = trace.series[a];
    const TraceSeries &sb = trace.series[b];

    if((sa.pid < 0) != (sb.pid < 0))
      return sa.pid < 0;
    if(sa.pid != sb.pid)
      return sa.pid < sb.pid;
    if(sa.group != sb.group)
      return sa.group < sb.group;

    return sa.name < sb.name;
  });

  for(int seriesIdx : seriesOrder)
  {
    TraceTrack track;
    track.kind = TraceTrackKind::Counter;
    track.name = trace.series[seriesIdx].name;
    track.unit = trace.series[seriesIdx].unit;
    track.counterSeries = seriesIdx;
    track.pid = trace.series[seriesIdx].pid;
    track.group = trace.series[seriesIdx].group;
    if(track.group.isEmpty())
      track.group = QObject::tr("Metrics");
    trace.tracks.push_back(track);
  }

  // Per-CPU lanes next: which thread occupied each CPU. They are kept as their own group because
  // they answer "what was this CPU doing" rather than "where did this thread run".
  for(int cpu = 0; cpu < builder.cpuLanes.size(); cpu++)
  {
    RawLane &lane = builder.cpuLanes[cpu];
    if(lane.spans.isEmpty())
      continue;

    AssignNestingDepth(lane.spans);

    TraceTrack track;
    track.kind = TraceTrackKind::Cpu;
    track.cpu = cpu;
    track.name = QObject::tr("Sched CPU %1").arg(cpu);
    track.group = QObject::tr("CPU Scheduling");
    track.spans = lane.spans;
    trace.tracks.push_back(track);
  }

  // Interrupt and softirq rows sit next to the scheduler rows, one per CPU, so "the CPU was taken
  // away from the app" is visible on the same axis as "the app was running".
  if(!builder.irqLanes.isEmpty())
  {
    for(int cpu = 0; cpu < builder.irqLanes.size(); cpu++)
    {
      RawLane &lane = builder.irqLanes[cpu];
      if(lane.spans.isEmpty())
        continue;

      AssignNestingDepth(lane.spans);

      TraceTrack track;
      track.kind = TraceTrackKind::Cpu;
      track.cpu = cpu;
      track.name = QObject::tr("IRQ CPU %1").arg(cpu);
      track.group = QObject::tr("Interrupts");
      track.spans = lane.spans;
      trace.tracks.push_back(track);
    }
  }

  for(int cpu = 0; cpu < builder.softirqLanes.size(); cpu++)
  {
    RawLane &lane = builder.softirqLanes[cpu];
    if(lane.spans.isEmpty())
      continue;

    AssignNestingDepth(lane.spans);

    TraceTrack track;
    track.kind = TraceTrackKind::Cpu;
    track.cpu = cpu;
    track.name = QObject::tr("SoftIRQ CPU %1").arg(cpu);
    track.group = QObject::tr("Interrupts");
    track.spans = lane.spans;
    trace.tracks.push_back(track);
  }

  // Order lanes by pid then tid so the group headers stay contiguous; lanes without ids sort last
  // and are named as unknown rather than being assigned a fabricated id.
  QVector<int> order;
  order.reserve(builder.lanes.size());
  for(int i = 0; i < builder.lanes.size(); i++)
    order.push_back(i);

  std::sort(order.begin(), order.end(), [&](int a, int b) {
    const RawLane &la = builder.lanes[a];
    const RawLane &lb = builder.lanes[b];

    if(la.pid != lb.pid)
    {
      if(la.pid < 0)
        return false;
      if(lb.pid < 0)
        return true;
      return la.pid < lb.pid;
    }

    if(la.tid != lb.tid)
    {
      if(la.tid < 0)
        return false;
      if(lb.tid < 0)
        return true;
      return la.tid < lb.tid;
    }

    return a < b;
  });

  for(int laneIdx : order)
  {
    RawLane &lane = builder.lanes[laneIdx];

    if(lane.spans.isEmpty())
      continue;

    AssignNestingDepth(lane.spans);

    TraceTrack track;
    track.pid = lane.pid;
    track.tid = lane.tid;
    track.kind = TraceTrackKind::Thread;

    // Names come only from metadata the trace itself declared. When it declared none, the caption
    // names the ids it did declare, which is truthful and still lets the user correlate a lane with
    // the trace's own pid/tid.
    const QString threadName = lane.tid >= 0 ? builder.threadNames.value(lane.tid) : QString();
    const QString processName = lane.pid >= 0 ? builder.processNames.value(lane.pid) : QString();
    // A raw systrace export declares no thread metadata at all, but every sched_switch names both
    // threads it moved, so the comm of a thread is still known - it is just the last name it carried
    // rather than one fixed name. A declared `thread_name` always wins over it.
    const QString commName = lane.tid >= 0 ? builder.commByTid.value(lane.tid) : QString();

    if(!threadName.isEmpty())
      track.name = threadName;
    else if(!commName.isEmpty())
      track.name = commName;
    else if(lane.atrace && lane.pid >= 0 && lane.pid == lane.tid)
      track.name = processName.isEmpty() ? QObject::tr("atrace pid %1").arg(lane.pid) : processName;
    else if(lane.tid >= 0)
      track.name = QObject::tr("tid %1").arg(lane.tid);
    else
      track.name = QObject::tr("Unknown thread");

    if(!processName.isEmpty())
      track.group = processName;
    else if(lane.pid >= 0 && lane.pid != lane.tid)
      track.group = QObject::tr("pid %1").arg(lane.pid);
    else
      track.group = QObject::tr("Other threads");

    // atrace marks are emitted by a process, not by an individual thread, so a lane made from
    // them is a process lane even when metadata named the pid.
    if(lane.atrace && lane.pid == lane.tid)
      track.kind = TraceTrackKind::Process;
    else if(threadName.isEmpty() && processName.isEmpty() && lane.tid < 0 && lane.pid < 0)
      track.kind = TraceTrackKind::Process;

    track.atrace = lane.atrace;
    track.spans = lane.spans;
    trace.tracks.push_back(track);
  }

  // A Chrome trace from `traceconv` declares no frame boundaries at all: the source has no notion
  // of a captured frame. Derive them from the per-frame markers that are present so correlation has
  // something to align against.
  if(trace.frameBoundaries.isEmpty())
  {
    DeriveFrameBoundaries(trace);

    if(!trace.frameBoundaries.isEmpty())
    {
      trace.warnings << QObject::tr(
          "%1 frame boundaries were derived from '%2' markers: they are numbered as ordinals "
          "within this trace, not frame ids the source declared")
                            .arg(trace.frameBoundaries.size())
                            .arg(trace.frameBoundaries.first().source);
    }
  }

  if(stats.counters == 0 && trace.series.isEmpty() && !trace.tracks.isEmpty())
    trace.warnings << QObject::tr(
        "Trace contained no counter events: only spans and frame boundaries can be shown");

  if(stats.framesFromName > 0)
    trace.warnings << QObject::tr(
        "%1 frame boundaries were identified from a 'Frame <n>' slice name rather than an "
        "explicit frame index in 'args'")
                          .arg(stats.framesFromName);

  if(stats.malformed > 0)
    trace.warnings << QObject::tr("%1 events could not be parsed and were skipped")
                          .arg(stats.malformed);

  if(stats.unusableCounters > 0)
    trace.warnings << QObject::tr(
        "%1 counter events carried no named values and were skipped")
                          .arg(stats.unusableCounters);

  if(stats.missingDuration > 0)
    trace.warnings << QObject::tr(
        "%1 complete ('ph':'X') events had no 'dur', so they could not be drawn as spans")
                          .arg(stats.missingDuration);

  if(stats.unterminatedSpans > 0)
    trace.warnings << QObject::tr(
        "%1 end events had no matching begin event and were skipped")
                          .arg(stats.unterminatedSpans);

  if(closedUnterminated > 0)
    trace.warnings << QObject::tr(
        "%1 spans were still open at the end of the trace and were closed at the last recorded "
        "timestamp: their true end is unknown")
                          .arg(closedUnterminated);

  if(stats.reversedSpans > 0)
    trace.warnings << QObject::tr("%1 spans ended before they began and were skipped")
                          .arg(stats.reversedSpans);

  if(stats.metadataUnusable > 0)
    trace.warnings << QObject::tr(
        "%1 metadata records carried no usable name or no id, so lanes keep their id captions")
                          .arg(stats.metadataUnusable);

  if(!builder.unmodelledEvents.isEmpty())
  {
    // Naming the first few is actionable; a full list of every kernel tracepoint would not be.
    QStringList names;
    int total = 0;
    for(auto it = builder.unmodelledEvents.constBegin(); it != builder.unmodelledEvents.constEnd();
        ++it)
    {
      total++;
      if(names.size() < 6)
        names << it.key();
    }

    std::sort(names.begin(), names.end());

    trace.warnings << QObject::tr(
        "%1 ftrace event type(s) have no lane or metric in this build and are not drawn: %2%3")
                          .arg(total)
                          .arg(names.join(QStringLiteral(", ")))
                          .arg(total > names.size() ? QStringLiteral(", ...") : QString());
  }

  if(stats.droppedClockSeries > 0)
    trace.warnings << QObject::tr(
        "%1 clock series belonged to more than %2 distinct clocks and were left out of the "
        "'Clocks' group")
                          .arg(stats.droppedClockSeries)
                          .arg(MaxClockSeries);

  if(systraceUsed)
  {
    trace.warnings << QObject::tr(
        "Timeline rebuilt from the Systrace (ftrace) text in 'systemTraceEvents': %1 events "
        "contributed %2 spans (running, runnable, sleeping, interrupt and workqueue) across %3 "
        "lanes, %4 metric series and %5 metric samples")
                          .arg(systraceEventCount)
                          .arg(trace.SpanCount())
                          .arg(trace.tracks.size() - trace.series.size())
                          .arg(trace.series.size())
                          .arg(builder.stats.systraceCounterSamples);

    // A trace full of scheduler evidence but containing no atrace mark at all is not a quiet app:
    // the device did not enable atrace, so there are no userspace lanes and no frame callback to
    // derive boundaries from. Saying so is the difference between "the panel is broken" and "record
    // it again".
    if(builder.stats.atraceMarks == 0)
      trace.warnings << QObject::tr(
          "The trace contains no atrace marks (`tracing_mark_write`), so there are no userspace "
          "lanes and no `Choreographer#doFrame` boundaries. The recording itself is fine; atrace "
          "was not enabled on the device while it ran, which happens when `traced_probes` is not "
          "tagging the app. Recording again usually produces them.");
  }

  if(trace.series.isEmpty() && trace.frameBoundaries.isEmpty() && trace.SpanCount() == 0)
  {
    trace.warnings << QObject::tr(
        "Trace contained no counter events, spans and no frame boundaries, so nothing can be "
        "correlated (%1 events examined, %2 phases not drawn)")
                          .arg(stats.events)
                          .arg(stats.unmappedPhase);
    return trace;
  }

  if(trace.frameBoundaries.isEmpty())
    trace.warnings << QObject::tr(
        "Trace contained no frame boundaries: correlation to the capture will be limited to "
        "absolute time");

  trace.valid = true;
  return trace;
}
SystemTrace ImportChromeTraceEventFormat(const QString &path)
{
  SystemTrace trace;
  trace.source = path;
  trace.formatName = QStringLiteral("Chrome Trace Event Format (Perfetto traceconv json)");
  // Timestamps are assumed to be microseconds of device time; there is no shared clock with the
  // capture, so correlation has to derive an offset from frame boundaries.
  trace.timeUnit = QStringLiteral("s");

  QFile file(path);
  if(!file.open(QIODevice::ReadOnly))
  {
    trace.warnings << QObject::tr("Could not open trace file: %1").arg(path);
    return trace;
  }

  // A Chrome trace is one JSON document, but the events array can be far larger than is sensible
  // to materialise at once. The file is therefore scanned for balanced top-level event objects and
  // each one is parsed individually. This also means a single malformed event does not discard
  // the whole trace.
  const QByteArray contents = file.readAll();
  if(contents.isEmpty())
  {
    trace.warnings << QObject::tr("Trace file is empty: %1").arg(path);
    return trace;
  }

  // The builder accumulates everything, including metadata that captions lanes created by events
  // encountered earlier in the file.
  ChromeTraceBuilder builder;

  // Locate the event stream: either the value of "traceEvents", or the file's top-level array.
  int pos = 0;
  bool sawKey = false;

  {
    const int keyIdx = contents.indexOf("\"traceEvents\"");
    if(keyIdx >= 0)
    {
      pos = keyIdx + int(strlen("\"traceEvents\""));
      sawKey = true;
    }
  }

  // Advance to the opening bracket of the array.
  while(pos < contents.size() && contents.at(pos) != '[')
    pos++;

  if(pos >= contents.size())
  {
    trace.warnings << QObject::tr(
        "Trace does not contain a Chrome trace event array (expected a top-level array or a "
        "'traceEvents' key): %1")
                          .arg(path);
    return trace;
  }

  pos++; // past '['

  // `depth` counts brace nesting of the events themselves. Strings are tracked unconditionally
  // because event payloads routinely contain '[' and ']' inside names and arguments, and treating
  // those as structure would corrupt the depth count.
  int depth = 0;
  int objectStart = -1;
  bool inString = false;
  bool escaped = false;

  for(; pos < contents.size(); pos++)
  {
    const char c = contents.at(pos);

    if(inString)
    {
      if(escaped)
        escaped = false;
      else if(c == '\\')
        escaped = true;
      else if(c == '"')
        inString = false;
      continue;
    }

    if(c == '"')
    {
      inString = true;
      continue;
    }

    if(c == '{')
    {
      if(depth == 0)
        objectStart = pos;
      depth++;
      continue;
    }

    if(c == '}')
    {
      if(depth <= 0)
        continue;

      depth--;
      if(depth == 0 && objectStart >= 0)
      {
        QJsonParseError err = {};
        const QJsonDocument doc =
            QJsonDocument::fromJson(contents.mid(objectStart, pos - objectStart + 1), &err);

        if(err.error != QJsonParseError::NoError || !doc.isObject())
          builder.stats.malformed++;
        else
          ConsumeChromeEvent(doc.object(), builder);

        objectStart = -1;
      }
      continue;
    }

    // The event array's closing bracket at brace depth 0 ends the stream.
    if(c == ']' && depth == 0)
      break;
  }

  // traceconv json exports Android kernel events as one Systrace string, not as individual Chrome
  // events. Preserve its timeline when `traceEvents` contained no usable events.
  // The fallback has to key off "nothing drawable was built" rather than off the event count:
  // `traceconv json` emits the scheduler stream as ftrace text in `systemTraceEvents` and leaves
  // `traceEvents` holding nothing but a couple of hundred metadata records, which are counted as
  // events but contribute no lanes. Testing `events == 0` therefore never fires for a real export.
  const bool nothingDrawable =
      builder.series.isEmpty() && builder.frames.isEmpty() && builder.stats.spans == 0 &&
      builder.stats.counters == 0;

  bool systraceUsed = false;
  int systraceEventCount = 0;

  if(builder.stats.events == 0 || nothingDrawable)
  {
    QJsonParseError documentError = {};
    const QJsonDocument document = QJsonDocument::fromJson(contents, &documentError);
    if(documentError.error == QJsonParseError::NoError && document.isObject())
    {
      const QString ftrace = document.object().value(QStringLiteral("systemTraceEvents")).toString();
      if(!ftrace.isEmpty() && ConsumeSystemTraceEvents(ftrace, builder))
      {
        systraceUsed = true;
        systraceEventCount = builder.stats.systraceEvents;
        trace.formatName =
            QStringLiteral("Chrome Trace Event Format (Perfetto traceconv json/systrace)");
      }
    }
  }

  return BuildTraceFromBuilder(builder, path, trace.formatName, systraceUsed,
                               systraceEventCount, sawKey);
}

// A raw systrace text export (`traceconv systrace`, or `atrace`/`perfetto --txt` output) is the same
// event stream the JSON export embeds in `systemTraceEvents`, only without the JSON wrapper. Parsing
// it directly avoids materialising a multi-megabyte JSON string full of escaped newlines, and it is
// the only shape that carries the per-line `comm-tid` identity of every thread.
SystemTrace ImportSystraceText(const QString &path)
{
  SystemTrace trace;
  trace.source = path;

  QFile file(path);
  if(!file.open(QIODevice::ReadOnly))
  {
    trace.warnings << QObject::tr("Could not open trace file: %1").arg(path);
    return trace;
  }

  const QString text = QString::fromUtf8(file.readAll());

  ChromeTraceBuilder builder;
  if(!ConsumeSystemTraceEvents(text, builder))
  {
    trace.formatName = QStringLiteral("Systrace (ftrace text)");
    trace.timeUnit = QStringLiteral("s");
    trace.warnings << QObject::tr(
        "No Systrace events were found in the file: it contains no 'comm-tid (tgid) [cpu] "
        "timestamp: event:' lines");
    return trace;
  }

  return BuildTraceFromBuilder(builder, path, QStringLiteral("Systrace (ftrace text)"), true,
                               builder.stats.systraceEvents,
                               /* sawEventArray */ true);
}

// Picks the importer from the file's own content rather than from its extension: `traceconv` writes
// either a JSON document or a systrace text file and the caller is not required to know which.
SystemTrace ImportSystemTraceFile(const QString &path)
{
  QFile file(path);
  if(!file.open(QIODevice::ReadOnly))
  {
    SystemTrace trace;
    trace.source = path;
    trace.warnings << QObject::tr("Could not open trace file: %1").arg(path);
    return trace;
  }

  const QByteArray head = file.read(4096);
  file.close();

  // A systrace export starts with the kernel trace header, possibly after a blank line; a JSON
  // export starts with '{' or '['.
  const QByteArray trimmed = head.trimmed();
  if(trimmed.startsWith('{') || trimmed.startsWith('['))
    return ImportChromeTraceEventFormat(path);

  return ImportSystraceText(path);
}

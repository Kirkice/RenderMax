// Shared legacy summary; preserves the MCP JSON contract for offline CLI.
#pragma once
namespace TraceAnalysis
{
  // One series' samples, downsampled to fit in a tool response while keeping the envelope. Emitting
  // every sample of a 20-second trace would be a multi-megabyte reply that says less than a bounded
  // one: what a reader of a frequency or temperature curve needs is where it sat and where it spiked,
  // which the per-bucket min/max preserves and a mean would not.
inline QJsonArray BucketSamples(const TraceSeries &s, double windowBegin, double windowEnd, int maxPoints)
  {
    QJsonArray out;

    int first = 0;
    const int count = s.samples.size();
    while(first < count && s.samples[first].timeSec < windowBegin) first++;
    int last = first;
    while(last < count && s.samples[last].timeSec <= windowEnd) last++;
    if(last <= first)
      return out;

    auto appendSample = [&out](const TraceSample &sample, double lo, double hi) {
      out.append(QJsonObject{{QStringLiteral("t"), sample.timeSec},
          {QStringLiteral("v"), sample.value}, {QStringLiteral("lo"), lo},
          {QStringLiteral("hi"), hi}});
    };

    const int total = last - first;
    if(total <= maxPoints)
    {
      for(int i = first; i < last; i++)
        appendSample(s.samples[i], s.samples[i].value, s.samples[i].value);
      return out;
    }

    // Single pass, one bucket at a time, so this stays linear in the sample count rather than
    // buckets times samples: a one-million-sample clock series is normal on a phone trace.
    const double width = (windowEnd - windowBegin) / double(maxPoints);
    int i = first;
    for(int b = 0; b < maxPoints && i < last; b++)
    {
      const double bucketEnd = b == maxPoints - 1 ? windowEnd : windowBegin + width * (b + 1);
      bool any = false;
      double lo = 0.0, hi = 0.0;
      const TraceSample *lastInBucket = NULL;
      while(i < last && s.samples[i].timeSec <= bucketEnd)
      {
        const TraceSample &sample = s.samples[i];
        if(!any)
        {
          lo = hi = sample.value;
          any = true;
        }
        else
        {
          lo = qMin(lo, sample.value);
          hi = qMax(hi, sample.value);
        }
        lastInBucket = &sample;
        i++;
      }
      if(any)
        appendSample(*lastInBucket, lo, hi);
    }

    // Anything left over from floating-point edges joins the final bucket rather than being dropped.
    if(i < last && !out.isEmpty())
    {
      QJsonObject back = out.last().toObject();
      double lo = back.value(QStringLiteral("lo")).toDouble();
      double hi = back.value(QStringLiteral("hi")).toDouble();
      for(; i < last; i++)
      {
        lo = qMin(lo, s.samples[i].value);
        hi = qMax(hi, s.samples[i].value);
      }
      back[QStringLiteral("lo")] = lo;
      back[QStringLiteral("hi")] = hi;
      out.last() = back;
    }

    return out;
  }


inline QJsonObject LegacyAnalyze(const SystemTrace &trace, const QJsonObject &args, double begin, double end, double extentBegin, double extentEnd)
{
    const bool includeSpans = args.value(QStringLiteral("includeSpans")).toBool(false);
    const bool includeSamples = args.value(QStringLiteral("includeSamples")).toBool(false);
    const int maxSpansPerTrack = qBound(1, args.value(QStringLiteral("maxSpansPerTrack")).toInt(100), 5000);
    const int maxSamplesPerSeries = qBound(2, args.value(QStringLiteral("maxSamplesPerSeries")).toInt(120), 5000);
    const QString trackFilter = args.value(QStringLiteral("trackFilter")).toString();
    const QString seriesFilter = args.value(QStringLiteral("seriesFilter")).toString();
    auto matches = [](const QString &name, const QString &filter) {
      return filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive);
    };

    QJsonArray series, tracks, warnings;
    for(const TraceSeries &s : trace.series)
    {
      if(!matches(s.name, seriesFilter))
        continue;

      int samplesInWindow = 0;
      for(const TraceSample &sample : s.samples)
      {
        if(sample.timeSec < begin)
          continue;
        if(sample.timeSec > end)
          break;
        samplesInWindow++;
      }

      const QJsonObject windowStats = TraceAnalysis::SeriesStats(s, begin, end);
      QJsonObject entry{{QStringLiteral("name"), s.name}, {QStringLiteral("unit"), s.unit},
          {QStringLiteral("group"), s.group}, {QStringLiteral("pid"), s.pid},
          {QStringLiteral("step"), s.step}, {QStringLiteral("sampleCount"), s.samples.size()},
          {QStringLiteral("samplesInWindow"), samplesInWindow}, {QStringLiteral("min"), windowStats.value(QStringLiteral("min"))},
          {QStringLiteral("max"), windowStats.value(QStringLiteral("max"))},
          {QStringLiteral("stats"), windowStats},
          {QStringLiteral("statisticsScope"), QStringLiteral("requested window")}};

      if(includeSamples)
      {
        entry[QStringLiteral("samples")] = BucketSamples(s, begin, end, maxSamplesPerSeries);
        // True when the array is a downsampled envelope rather than every reading.
        entry[QStringLiteral("samplesDownsampled")] = samplesInWindow > maxSamplesPerSeries;
      }

      series.append(entry);
    }

    for(const TraceTrack &t : trace.tracks)
    {
      if(!matches(t.name, trackFilter))
        continue;

      int spansInWindow = 0;
      for(const TraceSpan &span : t.spans)
      {
        if(span.endSec < begin || span.beginSec > end)
          continue;
        spansInWindow++;
      }

      QJsonObject entry{{QStringLiteral("name"), t.name}, {QStringLiteral("group"), t.group},
          {QStringLiteral("kind"), int(t.kind)}, {QStringLiteral("pid"), t.pid},
          {QStringLiteral("tid"), t.tid}, {QStringLiteral("cpu"), t.cpu},
          {QStringLiteral("spanCount"), t.spans.size()},
          {QStringLiteral("spansInWindow"), spansInWindow},
          {QStringLiteral("counterSeries"), t.counterSeries}, {QStringLiteral("atrace"), t.atrace},
          {QStringLiteral("idle"), t.idle}};

      if(includeSpans && spansInWindow > 0)
      {
        QJsonArray spans;
        for(const TraceSpan &span : t.spans)
        {
          if(span.endSec < begin || span.beginSec > end)
            continue;
          if(spans.size() >= maxSpansPerTrack)
            break;
          QJsonObject item{{QStringLiteral("name"), span.name},
              {QStringLiteral("beginSec"), span.beginSec},
              {QStringLiteral("endSec"), span.endSec},
              {QStringLiteral("durationSec"), span.endSec - span.beginSec},
              {QStringLiteral("depth"), span.depth}};
          if(!span.category.isEmpty())
            item[QStringLiteral("category")] = span.category;
          if(!span.fields.isEmpty())
            item[QStringLiteral("fields")] = QJsonArray::fromStringList(span.fields);
          spans.append(item);
        }
        entry[QStringLiteral("spans")] = spans;
        // What was cut, so a truncated list is never mistaken for the whole lane.
        entry[QStringLiteral("spansOmitted")] = spansInWindow - spans.size();
      }

      tracks.append(entry);
    }

    for(const QString &warning : trace.warnings) warnings.append(warning);

    const QVector<TraceThreadTime> times = trace.ThreadTimes(begin, end);
    QJsonArray threadTimes;
    for(const TraceThreadTime &row : times)
      threadTimes.append(QJsonObject{{QStringLiteral("name"), row.name}, {QStringLiteral("process"), row.process},
          {QStringLiteral("tid"), row.tid}, {QStringLiteral("runningSec"), row.runningSec},
          {QStringLiteral("runnableSec"), row.runnableSec}, {QStringLiteral("sleepingSec"), row.sleepingSec},
          {QStringLiteral("switches"), row.switches}});

    QJsonArray frames;
    int framesMeasured = 0;
    for(const TraceFrameBoundary &b : trace.frameBoundaries)
    {
      if(b.timeSec < begin || b.timeSec > end) continue;
      // Null rather than 0 when the GPU counters could not measure the frame: a frame the GPU never
      // touched and a frame whose GPU time is unknown are different answers.
      const QJsonValue gpuBusy = b.gpuBusySec < 0.0 ? QJsonValue() : QJsonValue(b.gpuBusySec);
      if(b.gpuBusySec >= 0.0)
        framesMeasured++;
      frames.append(QJsonObject{{QStringLiteral("frame"), b.source.isEmpty() ? QJsonValue(double(b.frameNumber)) : QJsonValue()},
          {QStringLiteral("frameNumberKnown"), b.source.isEmpty()},
          {QStringLiteral("timeSec"), b.timeSec}, {QStringLiteral("source"), b.source},
          {QStringLiteral("gpuBusySec"), gpuBusy},
          {QStringLiteral("gpuBusyScope"), QStringLiteral("complete frame interval, not clipped to query window")}});
    }

    // The GPU accounting, stated rather than left implicit in the frame table's dashes: whether the
    // counters were usable at all, and if not, which of the two reasons applies.
    const TraceGpuStats &gpu = trace.gpuStats;
    QJsonObject gpuSummary{{QStringLiteral("commandBatches"), gpu.commandBatches},
        {QStringLiteral("anchoredBatches"), gpu.anchoredBatches},
        {QStringLiteral("batchesWithoutCounters"), gpu.batchesWithoutCounters},
        {QStringLiteral("rejectedBatches"), gpu.rejectedBatches},
        {QStringLiteral("unterminatedBatches"), gpu.unterminatedBatches},
        {QStringLiteral("unterminatedWaits"), gpu.unterminatedWaits},
        {QStringLiteral("thermalZones"), gpu.thermalZones},
        {QStringLiteral("powerStateChanges"), gpu.powerStateChanges},
        {QStringLiteral("thermalTrips"), gpu.thermalTrips},
        {QStringLiteral("bandwidthOtherDevices"), gpu.bandwidthOtherDevices},
        {QStringLiteral("tickHz"), gpu.tickHz},
        {QStringLiteral("executionWindowsKnown"), gpu.HasExecutionWindows()},
        {QStringLiteral("framesWithGpuBusy"), framesMeasured},
        {QStringLiteral("accountingScope"), QStringLiteral("trace-wide except framesWithGpuBusy")},
        {QStringLiteral("framesWithGpuBusyScope"), QStringLiteral("frame boundaries in requested window")}};

    QString gpuBusyHint;
    if(framesMeasured == 0)
    {
      if(frames.isEmpty())
        gpuBusyHint = QStringLiteral("No frame boundaries in the requested window; per-frame GPU busy is unavailable even if GPU execution windows exist.");
      else if(gpu.commandBatches == 0 && gpu.unterminatedBatches == 0)
        gpuBusyHint = QStringLiteral("No GPU command batches observed; capture with gpuDetail commands. Event enablement alone does not guarantee events.");
      else if(gpu.rejectedBatches > 0)
        gpuBusyHint = QStringLiteral("GPU counter anchors were rejected; inspect tick clock and trace warnings.");
      else if(!gpu.HasExecutionWindows() && gpu.unterminatedBatches > 0)
        gpuBusyHint = QStringLiteral("No usable GPU execution windows; some command batches have no retirement. Inspect incomplete batches and counter availability.");
      else if(!gpu.HasExecutionWindows())
        gpuBusyHint = QStringLiteral("Observed batches have no usable GPU counter windows; inspect batchesWithoutCounters and trace warnings.");
      else
        gpuBusyHint = QStringLiteral("No measurable closed frame in this window; inspect frame boundaries and time range.");
    }

    QJsonArray unmodelled;
    for(const QPair<QString, int> &unmodelledEvent : trace.unmodelledEvents)
      unmodelled.append(QJsonObject{{QStringLiteral("event"), unmodelledEvent.first},
          {QStringLiteral("count"), unmodelledEvent.second}});

    return QJsonObject{
        {QStringLiteral("ok"), trace.valid}, {QStringLiteral("valid"), trace.valid},
        {QStringLiteral("source"), trace.source}, {QStringLiteral("format"), trace.formatName},
        {QStringLiteral("timeUnit"), trace.timeUnit}, {QStringLiteral("sharedClock"), trace.sharedClock},
        {QStringLiteral("startSec"), begin}, {QStringLiteral("endSec"), end},
        {QStringLiteral("traceExtentSec"), QJsonArray{extentBegin, extentEnd}},
        {QStringLiteral("windowed"), begin > extentBegin || end < extentEnd},
        {QStringLiteral("durationSec"), trace.SpanSec()},
        {QStringLiteral("durationScope"), QStringLiteral("whole trace; retained for compatibility")},
        {QStringLiteral("windowDurationSec"), end - begin},
        {QStringLiteral("threadTimesScope"), QStringLiteral("requested window")},
        {QStringLiteral("seriesCount"), trace.series.size()},
        {QStringLiteral("trackCount"), trace.tracks.size()}, {QStringLiteral("spanCount"), trace.SpanCount()},
        {QStringLiteral("frameBoundaryCount"), trace.frameBoundaries.size()},
        {QStringLiteral("gpu"), gpuSummary},
        {QStringLiteral("windowGpu"), TraceAnalysis::Gpu(trace, begin, end)},
        {QStringLiteral("gpuAccountingScope"), QStringLiteral("trace-wide batch counts; framesWithGpuBusy is windowed")},
        {QStringLiteral("gpuBusyHint"), gpuBusyHint},
        {QStringLiteral("series"), series}, {QStringLiteral("tracks"), tracks},
        {QStringLiteral("frames"), frames},
        {QStringLiteral("threadTimes"), threadTimes}, {QStringLiteral("warnings"), warnings},
        {QStringLiteral("unmodelledEvents"), unmodelled},
        {QStringLiteral("ftraceInventory"), QJsonArray::fromStringList(FtraceEventInventory())}};
  }

}

// System Trace analysis shared by native MCP session tools. Included inside namespace MCP.
// No replay/UI access: all results derive from the imported trace and explicit time windows.
namespace TraceAnalysis
{
inline double Overlap(double a, double b, double begin, double end)
{ return qMax(0.0, qMin(b, end) - qMax(a, begin)); }
inline QJsonValue Number(double v) { return std::isfinite(v) ? QJsonValue(v) : QJsonValue(); }
inline bool Match(const QString &name, const QString &filter)
{ return filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive); }
inline double Union(QVector<QPair<double, double>> intervals, double begin, double end)
{
  std::sort(intervals.begin(), intervals.end());
  double total = 0.0, right = begin;
  for(const auto &v : intervals)
  {
    const double a = qMax(begin, v.first), b = qMin(end, v.second);
    if(b > qMax(a, right)) total += b - qMax(a, right);
    right = qMax(right, b);
  }
  return total;
}
inline QJsonObject SeriesStats(const TraceSeries &s, double begin, double end)
{
  int count = 0, changes = 0;
  double lo = 0, hi = 0, sum = 0, first = 0, last = 0, integral = 0, covered = 0;
  QMap<double, double> residency;
  auto addRange = [&](double v) {
    if(count == 0) lo = hi = first = v;
    lo = qMin(lo, v); hi = qMax(hi, v); last = v; sum += v; count++;
  };
  for(int i = 0; i < s.samples.size(); i++)
  {
    const TraceSample &p = s.samples[i];
    if(!std::isfinite(p.value)) continue;
    if(p.timeSec >= begin && p.timeSec <= end) addRange(p.value);
    if(i > 0 && p.timeSec > begin && p.timeSec <= end && p.value != s.samples[i-1].value) changes++;
    if(s.step)
    {
      const double next = i + 1 < s.samples.size() ? s.samples[i+1].timeSec : end;
      const double dt = Overlap(p.timeSec, next, begin, end);
      if(dt > 0) { covered += dt; integral += dt * p.value; residency[p.value] += dt; }
    }
  }
  QJsonArray states;
  for(auto it = residency.constBegin(); it != residency.constEnd(); ++it)
    states.append(QJsonObject{{QStringLiteral("value"),it.key()},{QStringLiteral("durationSec"),it.value()}});
  // A carried-in level is observed evidence too, even with no transitions in the window.
  if(s.step && !residency.isEmpty())
  {
    lo = residency.firstKey(); hi = residency.lastKey();
  }
  const bool any = count > 0 || covered > 0;
  return QJsonObject{{QStringLiteral("sampleCount"),count},{QStringLiteral("min"),any ? QJsonValue(lo):QJsonValue()},
    {QStringLiteral("max"),any ? QJsonValue(hi):QJsonValue()},{QStringLiteral("sampleMean"),count ? QJsonValue(sum/count):QJsonValue()},
    {QStringLiteral("firstSample"),count ? QJsonValue(first):QJsonValue()},{QStringLiteral("lastSample"),count ? QJsonValue(last):QJsonValue()},
    {QStringLiteral("timeWeightedMean"),covered > 0 ? QJsonValue(integral/covered):QJsonValue()},
    {QStringLiteral("coveredSec"),s.step ? QJsonValue(covered):QJsonValue()},
    {QStringLiteral("coverage"),s.step && end > begin ? QJsonValue(covered/(end-begin)):QJsonValue()},
    {QStringLiteral("changes"),changes},{QStringLiteral("residency"),states},{QStringLiteral("step"),s.step},
    {QStringLiteral("method"),s.step ? QStringLiteral("last-observed level held to next sample or window end") : QStringLiteral("sample statistics; no continuity assumed")}};
}
inline QJsonArray Threads(const SystemTrace &trace, double begin, double end, const QJsonObject &args)
{
  QJsonArray rows;
  if(end <= begin) return rows;
  auto times = trace.ThreadTimes(begin,end);
  std::sort(times.begin(),times.end(),[](const TraceThreadTime &a,const TraceThreadTime &b){return a.runningSec > b.runningSec;});
  for(const auto &t : times)
  {
    if(args.contains(QStringLiteral("tid")) && t.tid != args.value(QStringLiteral("tid")).toInt()) continue;
    if(!Match(t.name,args.value(QStringLiteral("threadFilter")).toString())) continue;
    if(args.contains(QStringLiteral("pid")))
    {
      bool found = false;
      for(const auto &track : trace.tracks)
        if(track.tid == t.tid && track.pid == args.value(QStringLiteral("pid")).toInt()) found = true;
      if(!found) continue;
    }
    rows.append(QJsonObject{{QStringLiteral("name"),t.name},{QStringLiteral("process"),t.process},{QStringLiteral("tid"),t.tid},
      {QStringLiteral("runningSec"),t.runningSec},{QStringLiteral("runnableSec"),t.runnableSec},{QStringLiteral("sleepingSec"),t.sleepingSec},
      {QStringLiteral("switches"),t.switches},{QStringLiteral("oneCoreUtilization"),t.runningSec/(end-begin)}});
  }
  return rows;
}
inline QJsonObject Cpu(const SystemTrace &trace, double begin,double end,const QJsonObject &args)
{
  QJsonArray cores;
  for(const auto &track:trace.tracks)
  {
    if(track.kind != TraceTrackKind::Cpu || (args.contains(QStringLiteral("cpu")) && track.cpu != args.value(QStringLiteral("cpu")).toInt())) continue;
    QVector<QPair<double,double>> observed,busy;
    for(const auto &s:track.spans)
    {
      observed.append(qMakePair(s.beginSec,s.endSec));
      // CPU scheduler span name identifies the scheduled thread; the idle task is tid 0.
      bool idle = s.name == QStringLiteral("swapper") || s.name.startsWith(QStringLiteral("swapper/")) || s.name == QStringLiteral("idle");
      // Importer stores the scheduled thread identity in fields, not in the CPU track tid.
      for(const QString &f:s.fields) if(f == QStringLiteral("tid = 0")) idle = true;
      if(!idle) busy.append(qMakePair(s.beginSec,s.endSec));
    }
    double known=Union(observed,begin,end), run=Union(busy,begin,end);
    cores.append(QJsonObject{{QStringLiteral("cpu"),track.cpu},{QStringLiteral("observedSec"),known},{QStringLiteral("busySec"),run},
      {QStringLiteral("idleSec"),known-run},{QStringLiteral("unknownSec"),qMax(0.0,end-begin-known)},
      {QStringLiteral("utilizationOfObserved"),known>0 ? QJsonValue(run/known):QJsonValue()}});
  }
  QJsonArray threadRows=Threads(trace,begin,end,args);
  QMap<QString,double> processes;
  for(const auto &v:threadRows)
  {
    const auto row=v.toObject(); const QString process=row.value(QStringLiteral("process")).toString();
    if(!process.isEmpty())processes[process]+=row.value(QStringLiteral("runningSec")).toDouble();
  }
  QJsonArray processRows;
  for(auto it=processes.constBegin();it!=processes.constEnd();++it)
    processRows.append(QJsonObject{{QStringLiteral("process"),it.key()},{QStringLiteral("runningSec"),it.value()},
      {QStringLiteral("oneCoreUtilization"),end>begin?QJsonValue(it.value()/(end-begin)):QJsonValue()}});
  const int limit=qBound(1,args.value(QStringLiteral("limit")).toInt(100),5000);
  const int total=threadRows.size();while(threadRows.size()>limit)threadRows.removeLast();
  const int totalProcesses=processRows.size();while(processRows.size()>limit)processRows.removeLast();
  return QJsonObject{{QStringLiteral("cores"),cores},{QStringLiteral("threads"),threadRows},
    {QStringLiteral("threadRowsOmitted"),total-threadRows.size()},{QStringLiteral("processes"),processRows},
    {QStringLiteral("processRowsOmitted"),totalProcesses-processRows.size()},
    {QStringLiteral("processIdentityNote"),QStringLiteral("Grouped by declared process display name; unknown process identity is excluded, not guessed from tid.")},
    {QStringLiteral("utilizationDenominator"),QStringLiteral("per-core observed scheduler time; thread utilization uses one core times window duration")}};
}
inline QJsonObject Gpu(const SystemTrace &trace,double begin,double end)
{
  const double busy=Union(trace.gpuExecWindows,begin,end);
  const bool known=trace.gpuStats.HasExecutionWindows();
  const bool complete=known && trace.gpuStats.batchesWithoutCounters==0 && trace.gpuStats.rejectedBatches==0 && trace.gpuStats.unterminatedBatches==0;
  return QJsonObject{{QStringLiteral("knownExecutionSec"),known ? QJsonValue(busy):QJsonValue()},
    {QStringLiteral("busyLowerBoundSec"),known ? QJsonValue(busy):QJsonValue()},
    {QStringLiteral("uncoveredSec"),known ? QJsonValue(qMax(0.0,end-begin-busy)):QJsonValue()},
    {QStringLiteral("idleSec"),QJsonValue()},{QStringLiteral("utilizationLowerBound"),known && end>begin ? QJsonValue(busy/(end-begin)):QJsonValue()},
    {QStringLiteral("allRecordedBatchesAnchored"),complete},{QStringLiteral("coverageComplete"),false},
    {QStringLiteral("reason"),QStringLiteral("Uncovered intervals are not proven idle: trace event loss and work outside captured batches cannot be excluded.")},
    {QStringLiteral("traceWideAccounting"),QJsonObject{{QStringLiteral("commandBatches"),trace.gpuStats.commandBatches},{QStringLiteral("anchoredBatches"),trace.gpuStats.anchoredBatches},
      {QStringLiteral("withoutCounters"),trace.gpuStats.batchesWithoutCounters},{QStringLiteral("rejected"),trace.gpuStats.rejectedBatches},{QStringLiteral("unterminated"),trace.gpuStats.unterminatedBatches}}}};
}
inline QJsonArray Frames(const SystemTrace &trace,double begin,double end,const QJsonObject &args)
{
  QJsonArray rows;
  const double budget=args.value(QStringLiteral("frameBudgetMs")).toDouble(16.666667)/1000.0;
  for(int i=0;i<trace.frameBoundaries.size();i++)
  {
    const auto &f=trace.frameBoundaries[i];
    const bool closed=i+1<trace.frameBoundaries.size();
    const double stop=closed ? trace.frameBoundaries[i+1].timeSec:f.timeSec;
    if(f.timeSec>end || (closed ? stop<=begin:f.timeSec<begin)) continue;
    if(args.contains(QStringLiteral("frameIndex")) && args.value(QStringLiteral("frameIndex")).toInt()!=i) continue;
    if(args.contains(QStringLiteral("frameNumber")) && (!f.source.isEmpty() || double(f.frameNumber)!=args.value(QStringLiteral("frameNumber")).toDouble())) continue;
    const double duration=stop-f.timeSec;
    rows.append(QJsonObject{{QStringLiteral("frameIndex"),i},{QStringLiteral("frameNumber"),f.source.isEmpty()?QJsonValue(double(f.frameNumber)):QJsonValue()},
      {QStringLiteral("frameNumberKnown"),f.source.isEmpty()},{QStringLiteral("ordinal"),i+1},{QStringLiteral("source"),f.source},
      {QStringLiteral("beginSec"),f.timeSec},{QStringLiteral("endSec"),closed?QJsonValue(stop):QJsonValue()},
      {QStringLiteral("durationSec"),closed?QJsonValue(duration):QJsonValue()},
      {QStringLiteral("gpuBusySec"),f.gpuBusySec>=0?QJsonValue(f.gpuBusySec):QJsonValue()},
      {QStringLiteral("windowGpu"),closed?QJsonValue(Gpu(trace,qMax(begin,f.timeSec),qMin(end,stop))):QJsonValue()},
      {QStringLiteral("overBudget"),closed && duration>budget}});
  }
  return rows;
}
inline QJsonObject Page(const QJsonArray &rows,const QJsonObject &args)
{
  const int offset=qMax(0,args.value(QStringLiteral("offset")).toInt(0)),limit=qBound(1,args.value(QStringLiteral("limit")).toInt(100),5000);
  QJsonArray out;
  for(int i=offset;i<rows.size() && out.size()<limit;i++) out.append(rows[i]);
  return QJsonObject{{QStringLiteral("items"),out},{QStringLiteral("total"),rows.size()},{QStringLiteral("offset"),offset},{QStringLiteral("returned"),out.size()},
    {QStringLiteral("nextOffset"),offset+out.size()<rows.size()?QJsonValue(offset+out.size()):QJsonValue()}};
}
inline QJsonObject Analyze(const QString &tool,const SystemTrace &trace,const QJsonObject &args,double begin,double end)
{
  const QString op=tool.mid(QStringLiteral("renderdoc_").size());
  QJsonObject out{{QStringLiteral("ok"),trace.valid},{QStringLiteral("source"),trace.source},{QStringLiteral("beginSec"),begin},{QStringLiteral("endSec"),end},
    {QStringLiteral("durationSec"),end-begin},{QStringLiteral("statisticsScope"),QStringLiteral("requested window")},
    {QStringLiteral("warnings"),QJsonArray::fromStringList(trace.warnings)}};
  const bool report=op==QStringLiteral("buildSystemTraceReport");
  const int bucketCount=qBound(1,args.value(QStringLiteral("buckets")).toInt(1),1000);
  if(bucketCount>1 && (op==QStringLiteral("getSystemTraceCpuUtilization") || op==QStringLiteral("getSystemTraceGpuUtilization")))
  {
    QJsonArray buckets;
    for(int i=0;i<bucketCount;i++)
    {
      double a=begin+(end-begin)*double(i)/bucketCount,b=begin+(end-begin)*double(i+1)/bucketCount;
      buckets.append(QJsonObject{{QStringLiteral("beginSec"),a},{QStringLiteral("endSec"),b},
        {QStringLiteral("stats"),op.contains(QStringLiteral("Cpu"))?Cpu(trace,a,b,args):Gpu(trace,a,b)}});
    }
    out[QStringLiteral("buckets")]=buckets;
  }
  const bool overview=op==QStringLiteral("getSystemTraceCapabilities");
  if(overview || report)
  {
    int sched=0;
    for(const auto &t:trace.tracks) if(t.kind==TraceTrackKind::Cpu) sched++;
    out[QStringLiteral("capabilities")]=QJsonObject{{QStringLiteral("cpuScheduler"),sched>0},{QStringLiteral("gpuExecution"),trace.gpuStats.HasExecutionWindows()},
      {QStringLiteral("seriesCount"),trace.series.size()},{QStringLiteral("trackCount"),trace.tracks.size()},{QStringLiteral("frameCount"),trace.frameBoundaries.size()},
      {QStringLiteral("rawWakeupEventsRetained"),!trace.wakeups.isEmpty()},
      {QStringLiteral("liveGpuSamples"),trace.gpuStats.liveGpuSamples},{QStringLiteral("liveGpuWindowSamples"),trace.gpuStats.liveGpuWindowSamples},
      {QStringLiteral("note"),QStringLiteral("Capabilities describe observed model data, not device support or enabled events. liveGpuSamples counts the capture sampler's own utilisation/clock markers; zero means it did not run, not that the GPU was idle.")}};
  }
  if(op==QStringLiteral("getSystemTraceEventInventory") || overview)
  {
    QJsonArray unknown;
    for(const auto &v:trace.unmodelledEvents) unknown.append(QJsonObject{{QStringLiteral("event"),v.first},{QStringLiteral("count"),v.second}});
    out[QStringLiteral("supportedEventInventory")]=QJsonArray::fromStringList(FtraceEventInventory());
    out[QStringLiteral("unmodelledEvents")]=unknown;
    QJsonObject counts; for(auto it=trace.eventCounts.constBegin();it!=trace.eventCounts.constEnd();++it) counts[it.key()]=it.value();
    out[QStringLiteral("observedEventCounts")]=counts;
    out[QStringLiteral("inventoryNote")]=QStringLiteral("Event counts are trace-wide ftrace occurrences, not window counts; supported does not imply observed.");
  }
  if(op==QStringLiteral("analyzeSystemTraceCpu") || op==QStringLiteral("getSystemTraceCpuUtilization") || report) out[QStringLiteral("cpu")]=Cpu(trace,begin,end,args);
  if(op==QStringLiteral("getSystemTraceThreadTimes")) out[QStringLiteral("threads")]=Page(Threads(trace,begin,end,args),args);
  if(op==QStringLiteral("analyzeSystemTraceGpu") || op==QStringLiteral("getSystemTraceGpuUtilization") || report) out[QStringLiteral("gpu")]=Gpu(trace,begin,end);
  QString seriesFilter=args.value(QStringLiteral("seriesFilter")).toString();
  const bool seriesTool=op==QStringLiteral("querySystemTraceSeries") || op.contains(QStringLiteral("Frequency")) || op.contains(QStringLiteral("Bandwidth")) || op.contains(QStringLiteral("Thermal")) || op==QStringLiteral("analyzeSystemTracePower") || op==QStringLiteral("analyzeSystemTraceCpuIdle") || op==QStringLiteral("analyzeSystemTraceGpuQueue");
  if(seriesTool || report || op==QStringLiteral("analyzeSystemTraceCpu") || op==QStringLiteral("analyzeSystemTraceGpu"))
  {
    QJsonArray rows;
    for(const auto &s:trace.series)
    {
      if(!Match(s.name,seriesFilter) || !Match(s.group,args.value(QStringLiteral("groupFilter")).toString())) continue;
      if(args.contains(QStringLiteral("pid")) && s.pid!=args.value(QStringLiteral("pid")).toInt()) continue;
      const QString key=(s.name+QStringLiteral(" ")+s.group+QStringLiteral(" ")+s.unit).toLower();
      if(op.contains(QStringLiteral("CpuFrequency")) && (!key.contains(QStringLiteral("cpu")) || !key.contains(QStringLiteral("frequency")))) continue;
      if(op.contains(QStringLiteral("GpuFrequency")) && (!key.contains(QStringLiteral("gpu")) || !(key.contains(QStringLiteral("frequency")) || key.contains(QStringLiteral("clock"))))) continue;
      if(op.contains(QStringLiteral("Bandwidth")) && !(key.contains(QStringLiteral("bandwidth")) || key.contains(QStringLiteral("mb/s")))) continue;
      if(op.contains(QStringLiteral("Thermal")) && !(key.contains(QStringLiteral("temperature")) || key.contains(QStringLiteral("thermal")) || key.contains(QStringLiteral("°c")))) continue;
      if(op==QStringLiteral("analyzeSystemTracePower") && !(key.contains(QStringLiteral("power")) || key.contains(QStringLiteral("mw")))) continue;
      if(op==QStringLiteral("analyzeSystemTraceCpuIdle") && !(key.contains(QStringLiteral("cpu")) && key.contains(QStringLiteral("idle")))) continue;
      if(op==QStringLiteral("analyzeSystemTraceGpuQueue") && !key.contains(QStringLiteral("in flight"))) continue;
      QJsonObject row{{QStringLiteral("name"),s.name},{QStringLiteral("group"),s.group},{QStringLiteral("unit"),s.unit},{QStringLiteral("pid"),s.pid},{QStringLiteral("stats"),SeriesStats(s,begin,end)}};
      // Envelope output is globally bounded per series; no arithmetic averaging hides peaks.
      if(args.value(QStringLiteral("includeSamples")).toBool(false))
      {
        const int bins=qBound(2,args.value(QStringLiteral("maxSamplesPerSeries")).toInt(120),5000);
        QJsonArray samples;
        int index=0;
        while(index<s.samples.size() && s.samples[index].timeSec<begin) index++;
        for(int b=0;b<bins && index<s.samples.size();b++)
        {
          const double right=begin+(end-begin)*double(b+1)/bins;
          bool any=false;double lo=0,hi=0,time=0;
          while(index<s.samples.size() && s.samples[index].timeSec<=right)
          {
            const auto &p=s.samples[index++]; if(p.timeSec>end) break;
            if(!any) lo=hi=p.value;
            any=true;lo=qMin(lo,p.value);hi=qMax(hi,p.value);time=p.timeSec;
          }
          if(any) samples.append(QJsonObject{{QStringLiteral("timeSec"),time},{QStringLiteral("lo"),lo},{QStringLiteral("hi"),hi}});
        }
        row[QStringLiteral("samples")]=samples;row[QStringLiteral("sampleRepresentation")]=QStringLiteral("min/max envelope");
      }
      rows.append(row);
    }
    out[QStringLiteral("series")]=Page(rows,args);
  }
  if(op==QStringLiteral("querySystemTraceTracks") || op==QStringLiteral("getSystemTraceGpuBatches") || op==QStringLiteral("analyzeSystemTraceGpuWaits") || op==QStringLiteral("analyzeSystemTracePowerStates") || op==QStringLiteral("analyzeSystemTraceThermal") || op==QStringLiteral("analyzeSystemTraceScheduling") || op==QStringLiteral("analyzeSystemTraceWakeups"))
  {
    QJsonArray rows; QVector<double> delays;
    int migrations=0;QMap<int,QVector<QPair<double,int>>> runs;
    for(const auto &t:trace.tracks)
    {
      if(!Match(t.name,args.value(QStringLiteral("trackFilter")).toString())) continue;
      if(args.contains(QStringLiteral("pid")) && t.pid!=args.value(QStringLiteral("pid")).toInt()) continue;
      if(args.contains(QStringLiteral("tid")) && t.tid!=args.value(QStringLiteral("tid")).toInt()) continue;
      if(args.contains(QStringLiteral("cpu")) && t.cpu!=args.value(QStringLiteral("cpu")).toInt()) continue;
      if(op==QStringLiteral("getSystemTraceGpuBatches") && t.name!=QStringLiteral("GPU Commands")) continue;
      if(op==QStringLiteral("analyzeSystemTraceGpuWaits") && t.name!=QStringLiteral("GPU Waits")) continue;
      if(op==QStringLiteral("analyzeSystemTracePowerStates") && t.name!=QStringLiteral("GPU Power State")) continue;
      if(op==QStringLiteral("analyzeSystemTraceThermal") && t.name!=QStringLiteral("GPU Thermal Trips")) continue;
      for(const auto &s:t.spans)
      {
        double dt=Overlap(s.beginSec,s.endSec,begin,end);
        if(dt<=0 && !(s.beginSec==s.endSec && s.beginSec>=begin && s.beginSec<=end)) continue;
        if(dt<args.value(QStringLiteral("minDurationSec")).toDouble(0)) continue;
        if(op==QStringLiteral("analyzeSystemTraceScheduling") || op==QStringLiteral("analyzeSystemTraceWakeups"))
        {
          if(t.kind!=TraceTrackKind::Thread || t.atrace || !s.category.startsWith(QStringLiteral("sched"))) continue;
          if(op==QStringLiteral("analyzeSystemTraceWakeups") && s.category!=QStringLiteral("sched-runnable")) continue;
          if(s.category==QStringLiteral("sched-runnable")) delays.append(dt);
        }
        rows.append(QJsonObject{{QStringLiteral("track"),t.name},{QStringLiteral("group"),t.group},{QStringLiteral("kind"),int(t.kind)},
          {QStringLiteral("pid"),t.pid},{QStringLiteral("tid"),t.tid},{QStringLiteral("cpu"),t.cpu},{QStringLiteral("name"),s.name},{QStringLiteral("category"),s.category},
          {QStringLiteral("beginSec"),s.beginSec},{QStringLiteral("endSec"),s.endSec},{QStringLiteral("overlapSec"),dt},
          {QStringLiteral("fields"),QJsonArray::fromStringList(s.fields)}});
      }
    }
    if(op==QStringLiteral("analyzeSystemTraceScheduling"))
    {
      for(const auto &t:trace.tracks)
      {
        if(t.kind!=TraceTrackKind::Thread || t.atrace || t.idle)continue;
        if(args.contains(QStringLiteral("tid")) && t.tid!=args.value(QStringLiteral("tid")).toInt())continue;
        int previous=-1;
        for(const auto &s:t.spans)
        {
          if(s.category!=QStringLiteral("sched") || s.beginSec<begin || s.beginSec>end)continue;
          int cpu=-1;
          for(const QString &field:s.fields) if(field.startsWith(QStringLiteral("cpu = ")))cpu=field.mid(6).toInt();
          if(cpu>=0 && previous>=0 && cpu!=previous)migrations++;
          if(cpu>=0)previous=cpu;
        }
      }
      out[QStringLiteral("observedCpuMigrations")]=migrations;
      out[QStringLiteral("migrationMethod")]=QStringLiteral("Consecutive observed schedule-ins on different CPUs; interval boundaries may hide transitions.");
    }
    Q_UNUSED(runs);
    if(args.value(QStringLiteral("sortBy")).toString()==QStringLiteral("duration"))
    {
      QVector<QJsonValue> sorted; for(const auto &v:rows) sorted.append(v);
      std::sort(sorted.begin(),sorted.end(),[](const QJsonValue &a,const QJsonValue &b){return a.toObject().value(QStringLiteral("overlapSec")).toDouble()>b.toObject().value(QStringLiteral("overlapSec")).toDouble();});
      rows=QJsonArray();for(const auto &v:sorted)rows.append(v);
    }
    out[QStringLiteral("spans")]=Page(rows,args);
    if(op==QStringLiteral("analyzeSystemTraceWakeups") || op==QStringLiteral("analyzeSystemTraceScheduling"))
    {
      std::sort(delays.begin(),delays.end());double total=0;for(double d:delays) total+=d;
      out[QStringLiteral("runnableIntervals")]=QJsonObject{{QStringLiteral("count"),delays.size()},{QStringLiteral("totalSec"),total},
        {QStringLiteral("p95Sec"),delays.isEmpty()?QJsonValue():QJsonValue(delays[qMin(delays.size()-1,int(delays.size()*0.95))])}};
      out[QStringLiteral("wakeupLatencySec")]=QJsonValue();
      out[QStringLiteral("reason")]=QStringLiteral("Runnable intervals include wakeups and preemption. Raw wakeup provenance is not retained, so these are NOT wakeup latency measurements.");
    }
    if(op==QStringLiteral("analyzeSystemTracePowerStates"))
    {
      QMap<QString,double> states;for(const auto &v:rows){auto r=v.toObject();states[r.value(QStringLiteral("name")).toString()]+=r.value(QStringLiteral("overlapSec")).toDouble();}
      QJsonArray residency;for(auto i=states.constBegin();i!=states.constEnd();++i)residency.append(QJsonObject{{QStringLiteral("state"),i.key()},{QStringLiteral("durationSec"),i.value()}});
      out[QStringLiteral("residency")]=residency;
    }
  }
  if(op==QStringLiteral("analyzeSystemTraceGpuQueue") || op==QStringLiteral("getSystemTraceGpuBatches"))
  {
    QJsonArray batches; double delay=0;int measured=0;
    for(const auto &v:trace.batches)
    {
      if(v.retireSec<begin || (v.submitSec>=0 && v.submitSec>end)) continue;
      const bool valid=v.anchored && v.submitSec>=0 && v.beginSec>=v.submitSec;
      if(valid){delay+=v.beginSec-v.submitSec;measured++;}
      batches.append(QJsonObject{{QStringLiteral("context"),v.context},{QStringLiteral("timestamp"),v.timestamp},
        {QStringLiteral("submitSec"),v.submitSec>=0?QJsonValue(v.submitSec):QJsonValue()},
        {QStringLiteral("retireSec"),v.retireSec},{QStringLiteral("anchored"),v.anchored},
        {QStringLiteral("executionBeginSec"),v.anchored?QJsonValue(v.beginSec):QJsonValue()},
        {QStringLiteral("executionEndSec"),v.anchored?QJsonValue(v.endSec):QJsonValue()},
        {QStringLiteral("submitToStartSec"),valid?QJsonValue(v.beginSec-v.submitSec):QJsonValue()}});
    }
    out[QStringLiteral("batches")]=Page(batches,args);
    out[QStringLiteral("meanSubmitToStartSec")]=measured?QJsonValue(delay/measured):QJsonValue();
  }
  if(op==QStringLiteral("analyzeSystemTraceWakeups"))
  {
    QJsonArray rows;QVector<double> delays;
    QMap<int,QVector<double>> scheduleIns;
    for(const auto &t:trace.tracks)
      if(t.kind==TraceTrackKind::Thread && !t.atrace)
        for(const auto &s:t.spans)if(s.category==QStringLiteral("sched"))scheduleIns[t.tid].append(s.beginSec);
    for(auto it=scheduleIns.begin();it!=scheduleIns.end();++it)std::sort(it->begin(),it->end());
    for(const auto &wake:trace.wakeups)
    {
      if(wake.timeSec<begin || wake.timeSec>end || (args.contains(QStringLiteral("tid")) && wake.tid!=args.value(QStringLiteral("tid")).toInt())) continue;
      double run=-1;
      const auto schedules=scheduleIns.constFind(wake.tid);
      if(schedules!=scheduleIns.constEnd())
      {
        const auto next=std::lower_bound(schedules->constBegin(),schedules->constEnd(),wake.timeSec);
        if(next!=schedules->constEnd())run=*next;
      }
      const bool valid=run>=wake.timeSec && run<=end;
      if(valid)delays.append(run-wake.timeSec);
      rows.append(QJsonObject{{QStringLiteral("tid"),wake.tid},{QStringLiteral("targetCpu"),wake.cpu},
        {QStringLiteral("wakeupSec"),wake.timeSec},{QStringLiteral("runSec"),valid?QJsonValue(run):QJsonValue()},
        {QStringLiteral("latencySec"),valid?QJsonValue(run-wake.timeSec):QJsonValue()}});
    }
    std::sort(delays.begin(),delays.end());
    out[QStringLiteral("wakeups")]=Page(rows,args);
    out[QStringLiteral("p95WakeupLatencySec")]=delays.isEmpty()?QJsonValue():QJsonValue(delays[qMin(delays.size()-1,int(delays.size()*0.95))]);
    out[QStringLiteral("reason")]=QStringLiteral("Latency from first observed waking/wakeup transition to next observed schedule-in; unmatched or out-of-window endpoints remain unknown.");
  }
  if(op==QStringLiteral("getSystemTraceFrames") || op==QStringLiteral("analyzeSystemTraceFrame") || op==QStringLiteral("analyzeSystemTraceJank") || report)
  {
    QJsonArray frames=Frames(trace,begin,end,args), selected;
    for(const auto &f:frames) if(op!=QStringLiteral("analyzeSystemTraceJank") || f.toObject().value(QStringLiteral("overBudget")).toBool()) selected.append(f);
    if(args.value(QStringLiteral("sortBy")).toString()==QStringLiteral("duration") || op==QStringLiteral("analyzeSystemTraceJank"))
    {
      QVector<QJsonValue> sorted;for(const auto &v:selected)sorted.append(v);
      std::sort(sorted.begin(),sorted.end(),[](const QJsonValue &a,const QJsonValue &b){return a.toObject().value(QStringLiteral("durationSec")).toDouble()>b.toObject().value(QStringLiteral("durationSec")).toDouble();});
      selected=QJsonArray();for(const auto &v:sorted)selected.append(v);
    }
    QJsonObject page=Page(selected,args);
    if(op==QStringLiteral("analyzeSystemTraceJank"))
    {
      QJsonArray items=page.value(QStringLiteral("items")).toArray(), enriched;
      for(const auto &v:items)
      {
        auto frame=v.toObject();double a=qMax(begin,frame.value(QStringLiteral("beginSec")).toDouble()),b=qMin(end,frame.value(QStringLiteral("endSec")).toDouble(a));
        QJsonObject top=args;top[QStringLiteral("limit")]=5;
        frame[QStringLiteral("cpuEvidence")]=Cpu(trace,a,b,top);
        frame[QStringLiteral("diagnosis")]=QStringLiteral("Slow marker interval; inspect thread running/runnable costs and known GPU busy. No causal bottleneck asserted.");
        enriched.append(frame);
      }
      page[QStringLiteral("items")]=enriched;
    }
    out[QStringLiteral("frames")]=page;
    if(op==QStringLiteral("analyzeSystemTraceFrame"))
    {
      if(!args.contains(QStringLiteral("frameIndex")) && !args.contains(QStringLiteral("frameNumber"))) {out[QStringLiteral("ok")]=false;out[QStringLiteral("reason")]=QStringLiteral("Select frameIndex or declared frameNumber.");}
      else if(selected.size()==1)
      {
        auto f=selected[0].toObject();double a=qMax(begin,f.value(QStringLiteral("beginSec")).toDouble()),b=qMin(end,f.value(QStringLiteral("endSec")).toDouble(a));
        out[QStringLiteral("cpu")]=Cpu(trace,a,b,args);out[QStringLiteral("gpu")]=Gpu(trace,a,b);
        QJsonArray metrics;
        for(const auto &series:trace.series)metrics.append(QJsonObject{{QStringLiteral("name"),series.name},{QStringLiteral("unit"),series.unit},{QStringLiteral("stats"),SeriesStats(series,a,b)}});
        out[QStringLiteral("series")]=Page(metrics,args);
      }
    }
    out[QStringLiteral("frameSemantics")]=QStringLiteral("Marker spacing is not presentation time or GPU frame duration; overBudget identifies a slow marker interval, not proven missed presentation.");
  }
  if(op==QStringLiteral("compareSystemTraceWindows"))
  {
    const double a=args.value(QStringLiteral("compareBeginSec")).toDouble(begin),b=args.value(QStringLiteral("compareEndSec")).toDouble(end);
    out[QStringLiteral("baseline")]=QJsonObject{{QStringLiteral("cpu"),Cpu(trace,begin,end,args)},{QStringLiteral("gpu"),Gpu(trace,begin,end)}};
    out[QStringLiteral("comparison")]=QJsonObject{{QStringLiteral("beginSec"),a},{QStringLiteral("endSec"),b},{QStringLiteral("cpu"),Cpu(trace,a,b,args)},{QStringLiteral("gpu"),Gpu(trace,a,b)}};
    QJsonArray metrics;
    for(const auto &s:trace.series)
    {
      if(!Match(s.name,args.value(QStringLiteral("seriesFilter")).toString()))continue;
      metrics.append(QJsonObject{{QStringLiteral("name"),s.name},{QStringLiteral("unit"),s.unit},
        {QStringLiteral("baseline"),SeriesStats(s,begin,end)},{QStringLiteral("comparison"),SeriesStats(s,a,b)}});
    }
    out[QStringLiteral("seriesComparison")]=Page(metrics,args);
  }
  if(report) out[QStringLiteral("conclusions")]=QJsonArray{QStringLiteral("Ranked thread costs and over-budget marker intervals are evidence, not causal bottleneck diagnoses."),QStringLiteral("Inspect capability gaps and trace warnings before interpreting uncovered intervals as idle.")};
  return out;
}
}

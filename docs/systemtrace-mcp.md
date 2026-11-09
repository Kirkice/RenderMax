# System Trace MCP analysis

All analysis tools accept an existing absolute `filePath` (traceconv JSON or systrace text). Times are seconds in the trace clock, not relative to capture start. `beginSec`/`endSec` select a non-empty window.

## Tools

- `renderdoc_getSystemTraceCapabilities`: Observed CPU/GPU capabilities and data gaps; supported events are not proof of occurrence.
- `renderdoc_querySystemTraceSeries`: Paged metric queries with window statistics, step residency and optional bounded min/max envelopes.
- `renderdoc_querySystemTraceTracks`: Paged span queries, clipped overlap duration, original timestamps and source fields.
- `renderdoc_getSystemTraceEventInventory`: Supported ftrace schemas and unmodelled event counts. Modelled per-event counts are unavailable.
- `renderdoc_analyzeSystemTraceCpu`: CPU scheduler coverage, per-core busy/idle and ranked thread costs.
- `renderdoc_getSystemTraceCpuUtilization`: Per-core observed scheduler utilization and per-thread one-core window utilization.
- `renderdoc_getSystemTraceThreadTimes`: Window-clipped running, runnable and sleeping times per thread.
- `renderdoc_analyzeSystemTraceScheduling`: Scheduler intervals and runnable waiting; missing causal provenance is explicitly reported.
- `renderdoc_analyzeSystemTraceWakeups`: Runnable interval distributions; returns unknown wakeup latency when raw wakeup provenance is absent.
- `renderdoc_analyzeSystemTraceCpuFrequency`: CPU frequency sample and time-weighted residency statistics.
- `renderdoc_analyzeSystemTraceCpuIdle`: Observed CPU idle series and state residency.
- `renderdoc_analyzeSystemTraceGpu`: Validated GPU execution union and accounting, without inventing idle from missing events.
- `renderdoc_getSystemTraceGpuUtilization`: GPU known busy lower bound; uncovered time is not asserted idle.
- `renderdoc_getSystemTraceGpuBatches`: Paged GPU command batch spans with source evidence fields.
- `renderdoc_analyzeSystemTraceGpuQueue`: Observed inflight queue series; unavailable submit-to-start timing is explicit.
- `renderdoc_analyzeSystemTraceGpuWaits`: Paged GPU fence waits and window overlap durations.
- `renderdoc_analyzeSystemTraceGpuFrequency`: GPU frequency and clock level statistics and residency.
- `renderdoc_analyzeSystemTraceBandwidth`: Bandwidth statistics grouped by source device; CPU and GPU masters remain distinct.
- `renderdoc_analyzeSystemTracePower`: Power sample statistics; no energy extrapolation over unknown sampling gaps.
- `renderdoc_analyzeSystemTraceThermal`: Temperature samples and thermal trip events; correlations are not causal throttling proof.
- `renderdoc_analyzeSystemTracePowerStates`: Observed GPU power state intervals and state residency.
- `renderdoc_getSystemTraceFrames`: Frame marker intervals, declared frame identity and measured GPU busy.
- `renderdoc_analyzeSystemTraceFrame`: Select frameIndex or declared frameNumber and inspect CPU/GPU within that interval.
- `renderdoc_analyzeSystemTraceJank`: Over-budget marker intervals; marker spacing is not presentation timing.
- `renderdoc_compareSystemTraceWindows`: Compare CPU and GPU statistics for two explicit windows in one trace.
- `renderdoc_buildSystemTraceReport`: Window CPU/GPU, metrics and frame evidence with limitations, not speculative causal diagnoses.

## Query contract

`offset` (default 0), `limit` (default 100, max 5000) paginate detail arrays. `nextOffset: null` marks the last page. `seriesFilter`, `trackFilter`, `threadFilter`, `groupFilter` are case-insensitive substrings on applicable queries. `pid`, `tid`, `cpu` filter applicable identities. `sortBy: "duration"` sorts span overlap descending. `minDurationSec` filters overlap.

Series queries optionally accept `includeSamples: true`, `maxSamplesPerSeries` (2..5000); returned samples are min/max envelopes, not averages. Step series use carried-in values and hold the last observed level through window end; coverage does not prove absence of dropped events. Non-step series report sample statistics, not time averages or energy.

Utilization queries accept `buckets` (1..1000). CPU utilization uses observed scheduler time per core; per-thread one-core utilization divides running time by window duration. Unknown scheduler time is not idle. Process grouping uses declared display names, not inferred PID ownership.

Frame queries accept `frameIndex` (zero based), declared `frameNumber`, and `frameBudgetMs` (default 16.666667). Unknown source frame numbers are null; `ordinal` is not a capture frame number. Marker intervals are not proof of missed presentation. `gpuBusySec` refers to the complete marker interval; `windowGpu` is clipped.

Window comparison accepts `compareBeginSec`, `compareEndSec` inside the same trace.

## Evidence limits

GPU execution is a union of validated counter windows. Busy is a lower bound; uncovered time is not proven idle. Batch accounting and event occurrence counts are explicitly trace-wide. Queue latency requires typed anchored batch evidence. Wake latency uses the first observed waking/wakeup transition and next schedule-in; unmatched endpoints remain null. Runnable intervals also include preemption and must not be called wakeup delays.

Power is a kernel estimate where indicated, not a physical power measurement. Thermal correlation does not establish throttling. Unknown numeric power states and trip types remain numeric labels. Bandwidth masters remain separate devices.

`renderdoc_correlateSystemTrace` retains replay requirements, alignment confidence/tolerance and its containment-only semantics: it cannot prove a Draw belongs to a hardware batch.

## Examples

```json
{"filePath":"C:/traces/capture.systrace.txt","beginSec":1000.0,"endSec":1000.1,"limit":20}
```

Use with `renderdoc_analyzeSystemTraceCpu`, `renderdoc_analyzeSystemTraceGpu`, or `renderdoc_buildSystemTraceReport`. Query capabilities first; missing events are not zero activity.

## Implementation limitations

Import is synchronous; cancellation is checked before/after import and analysis, not inside the importer. A single trace up to 16 MiB source size is cached on the serialized session worker, keyed by path/size/mtime; this is not a parsed-memory hard cap. Basic reports provide measured evidence, not an automated causal bottleneck classifier.

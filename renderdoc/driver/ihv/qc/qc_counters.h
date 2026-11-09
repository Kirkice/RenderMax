/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2026 Baldur Karlsson
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

#include <map>
#include "api/replay/data_types.h"
#include "api/replay/renderdoc_replay.h"
#include "api/replay/replay_enums.h"
#include "common/common.h"

// Qualcomm / Adreno (GGPM) hardware counter backend.
//
// This backend is the RenderDoc-side, original implementation of the counter behaviour that
// Qualcomm's Snapdragon Profiler exposes as GGPM (the "Adreno GPU Metrics" group of counters).
// It does NOT link, bundle or redistribute any Qualcomm binary.
//
// Two device mechanisms were evaluated for reading the counters, and the choice matters:
//
//   * `GL_AMD_performance_monitor` (the Adreno driver's own perf-monitor extension) is what
//     actually returns values. The extension string is unreliable - on an Adreno 610 it reports
//     zero counters while the entry points are present and sample real data - so availability is
//     decided by resolving the entry points, not by looking at GL_EXTENSIONS.
//   * `libgsl.so`'s `gsl_perfcounter_*` accepts a select for every (group, id) pair but does not
//     write values back: the 4-argument read returns 0 with untouched memory, the 5-argument form
//     crashes, and the batch read leaves its value fields unmodified. Those entry points are
//     therefore used only to verify that a pair is addressable, never to read.
//
// The counters the profiler shows ("% Shaders Busy", "% Texture Pipes Busy", ...) are derived:
// each one is a formula over raw hardware block counters, computed by libGGPM.so. The raw
// counters and formulas live in qc_ggpm_metrics.h, transcribed from the decompiled library; the
// per-block counter name tables live in qc_ggpm_names.h, generated from the plugin's own name
// arrays. This file is the plumbing: it enumerates the driver's perf-monitor catalog, resolves the
// raw counters by name, samples them in as few passes as the per-block active limits allow, and
// evaluates the formulas over the sampled values.
//
// On non-Android builds no counters are available and the stub implementation is compiled instead
// (qc_counters_stub.cpp), mirroring the ARM backend.

class QCCounters
{
public:
  enum class ApiType
  {
    Ogl,
    Vk,
  };

  QCCounters();
  ~QCCounters();

  // Establish the device-side counter session. `renderer` is the GL_RENDERER string, used to
  // resolve the Adreno part number and with it the per-device constants the formulas need.
  bool Init(ApiType api, void *context, const char *renderer);
  rdcarray<GPUCounter> GetPublicCounterIds();
  CounterDescription GetCounterDescription(GPUCounter index);

  void EnableCounter(GPUCounter counter);
  void DisableAllCounters();

  uint32_t GetPassCount();

  void BeginPass(uint32_t passID);
  void EndPass();

  void BeginSample(uint32_t eventId);
  void EndSample();

  // True when every counter the current selection reads is already cached for this event, in which
  // case the sample is copied into this fetch's results and the caller can skip replaying it.
  // Measuring one event costs a full GPU sync on the device, which dominates the cost of a fetch,
  // so this is what makes a second fetch over the same frame cheap instead of a full re-measure.
  bool ReuseSample(uint32_t eventId);

  rdcarray<CounterResult> GetCounterData(const rdcarray<uint32_t> &eventIDs,
                                         const rdcarray<GPUCounter> &counters);

private:
#if ENABLED(RDOC_ANDROID)
  // The AMD_performance_monitor entry points, resolved through eglGetProcAddress. Declared with
  // plain C types so this header never has to include a GL header.
  struct PerfMonApi
  {
    void *module;
    void *gen_monitors;
    void *delete_monitors;
    void *get_groups;
    void *get_counters;
    void *get_group_string;
    void *get_counter_string;
    void *get_counter_info;
    void *select_counters;
    void *begin;
    void *end;
    void *get_counter_data;
    void *flush;
  };

  // One counter in the driver's own catalog, as enumerated from GL_AMD_performance_monitor.
  struct DeviceCounter
  {
    rdcarray<char> name;
    uint32_t group = 0;
    uint32_t counter = 0;
    uint32_t type = 0;
  };

  struct DeviceGroup
  {
    rdcarray<char> name;
    uint32_t group = 0;
    uint32_t maxActive = 0;
    rdcarray<DeviceCounter> counters;
  };

  // A raw counter the formulas read, resolved to the driver's addressing of it. `index` is the
  // position in kQCGGPMNeeds; several metrics read the same entry, which is why the needs table
  // rather than the metric table decides what is selected on the device.
  struct NeedBinding
  {
    uint32_t index = 0;
    uint32_t group = 0;
    uint32_t counter = 0;
    uint32_t maxActive = 0;
    uint32_t type = 0;
    bool resolved = false;
  };

  // A metric that is available on this device: a public GPUCounter and the metric it evaluates.
  struct BuiltMetric
  {
    GPUCounter id;
    uint32_t metric = 0;
    bool available = false;
  };

  bool LoadPerfMon();
  bool CreateGLContext();
  void DestroyGLContext();
  void MakeCurrent();
  void EnumerateCatalog();
  void ResolveNeeds();
  void BuildCatalog();
  void BuildPasses();
  void SampleDeviceClock();

  static const DeviceCounter *FindDeviceCounter(const rdcarray<DeviceGroup> &groups,
                                                const char *name, uint32_t *maxActive);

  // Deterministically map a metric key into the Qualcomm ID range, so a counter's public ID is
  // identical on every Adreno generation and does not depend on what the device happens to expose.
  static GPUCounter StableIdFromKey(const char *key);

  int32_t IndexForId(GPUCounter counter) const;

  bool ReadResults(const rdcarray<uint32_t> &needIndices, const uint32_t *words, size_t wordCount,
                   double *out);

  PerfMonApi m_Perf = {};

  // The driver's catalog, kept both for name resolution and for the diagnostics logged in Init().
  rdcarray<DeviceGroup> m_DeviceGroups;

  // kQCGGPMNeeds, resolved to device addressing. index order matches the needs table.
  rdcarray<NeedBinding> m_Needs;

  // Pass layout: each element is the set of need indices sampled in one pass. A pass exists
  // because a hardware block can only have so many counters active at once (the driver's
  // per-group maxActive), not because the caller asked for several passes.
  rdcarray<rdcarray<uint32_t>> m_Passes;

  ApiType m_Api = ApiType::Ogl;
  void *m_ApiContext = NULL;

  // When the counters are sampled from a Vulkan replay the perf-monitor extension still needs a
  // current GL context, so one is created inside the replay process purely to issue the sampling
  // calls. The counters themselves are hardware block activity, so the context that issues the
  // begin/end markers does not have to be the one the work was submitted from.
  bool m_OwnContext = false;
  void *m_EglDisplay = NULL;
  void *m_EglContext = NULL;
  void *m_EglSurface = NULL;
  void *m_EglModule = NULL;

  // [观测] The perf-monitor handle is a GLuint allocated by the driver, 0 when no monitor exists.
  uint32_t m_Monitor = 0;
  uint32_t m_OpenPass = 0;
  bool m_Sampling = false;
  uint32_t m_CurrentEvent = 0;
  double m_SampleStartSec = 0.0;

  // Derived from the renderer string; 0 when the part is not in the profiler's device table, in
  // which case every formula that scales by a pipe count reports no value rather than a guess.
  uint32_t m_AdrenoPart = 0;
  double m_MaxClockHz = 0.0;

  rdcarray<uint32_t> m_EnabledCounters;
  rdcarray<GPUCounter> m_CounterIds;
  rdcarray<BuiltMetric> m_Built;

  // Per event: the raw counter values in kQCGGPMNeeds order, and the wall duration of the window
  // the counters cover. A value of -1.0 means the counter was not measured in that window, which
  // is what makes an unmeasurable metric distinguishable from a real zero.
  struct EventSample
  {
    rdcarray<double> raw;
    double dt = 0.0;
    // One flag per entry of kQCGGPMNeeds: set once a pass has actually tried to measure that raw
    // counter for this event, whether or not the driver returned a record for it. "Tried" and
    // "returned a value" are different things - the driver drops records for counters that exceed
    // a block's active limit - and only "tried" decides whether a later fetch may reuse the event.
    rdcarray<char> attempted;
  };

  std::map<uint32_t, EventSample> m_Samples;

  // Samples are retained for the lifetime of the replay session, keyed by event ID. Replaying an
  // event ID always reproduces the same state inside one capture, so a counter measured once for an
  // event stays valid for every later fetch - including fetches of a different counter selection.
  // A value of -1.0 still means "not measured", which is what keeps a counter that a pass did not
  // cover distinguishable from a counter that was measured as zero.
  std::map<uint32_t, EventSample> m_SampleCache;
#endif
};

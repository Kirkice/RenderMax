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

#include "qc_counters.h"

#if ENABLED(RDOC_ANDROID)

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>

#include "qc_ggpm_metrics.h"

// GL_AMD_performance_monitor. The constants are repeated here rather than including a GL header:
// this file is also compiled for the Vulkan backend, and the counter plumbing has no other need
// for GL types.
enum
{
  GL_PERFMON_RESULT_AVAILABLE_AMD = 0x8BC4,
  GL_PERFMON_RESULT_SIZE_AMD = 0x8BC5,
  GL_PERFMON_RESULT_AMD = 0x8BC6,
  GL_COUNTER_TYPE_AMD = 0x8BC0,
  GL_COUNTER_RANGE_AMD = 0x8BC1,
  GL_UNSIGNED_BYTE = 0x1401,
  GL_INT = 0x1404,
  GL_UNSIGNED_INT = 0x1405,
  GL_FLOAT = 0x1406,
};

typedef void (*PFN_glGenPerfMonitorsAMD)(int n, uint32_t *monitors);
typedef void (*PFN_glDeletePerfMonitorsAMD)(int n, uint32_t *monitors);
typedef void (*PFN_glGetPerfMonitorGroupsAMD)(int32_t *numGroups, int32_t groupsSize,
                                              uint32_t *groups);
typedef void (*PFN_glGetPerfMonitorCountersAMD)(uint32_t group, int32_t *numCounters,
                                                int32_t *maxActive, int32_t countersSize,
                                                uint32_t *counters);
typedef void (*PFN_glGetPerfMonitorGroupStringAMD)(uint32_t group, int32_t bufSize,
                                                   int32_t *length, char *groupString);
typedef void (*PFN_glGetPerfMonitorCounterStringAMD)(uint32_t group, uint32_t counter,
                                                     int32_t bufSize, int32_t *length,
                                                     char *counterString);
typedef void (*PFN_glGetPerfMonitorCounterInfoAMD)(uint32_t group, uint32_t counter, uint32_t pname,
                                                   void *data);
typedef void (*PFN_glSelectPerfMonitorCountersAMD)(uint32_t monitor, uint8_t enable,
                                                   uint32_t group, int32_t numCounters,
                                                   uint32_t *counterList);
typedef void (*PFN_glBeginPerfMonitorAMD)(uint32_t monitor);
typedef void (*PFN_glEndPerfMonitorAMD)(uint32_t monitor);
typedef void (*PFN_glGetPerfMonitorCounterDataAMD)(uint32_t monitor, uint32_t pname,
                                                   int32_t dataSize, uint32_t *data,
                                                   int32_t *bytesWritten);

namespace {
double NowSeconds()
{
  timespec ts = {};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return double(ts.tv_sec) + double(ts.tv_nsec) * 1.0e-9;
}

// Case-insensitive comparison, used because the driver's spelling of a counter name and the
// profiler's name table are independent strings ("AXI0_READ_DATA_BEATS_TOTAL" and friends appear
// without the PERF_ prefix on GBIF).
bool NameEquals(const char *a, const char *b)
{
  if(a == NULL || b == NULL)
    return false;

  while(*a && *b)
  {
    char ca = *a, cb = *b;
    if(ca >= 'a' && ca <= 'z')
      ca = char(ca - 'a' + 'A');
    if(cb >= 'a' && cb <= 'z')
      cb = char(cb - 'a' + 'A');
    if(ca != cb)
      return false;
    a++;
    b++;
  }

  return *a == *b;
}

// Read one integer out of a /sys node, 0 when it is missing or unparsable.
uint64_t ReadSysUInt64(const char *path)
{
  FILE *f = fopen(path, "r");
  if(f == NULL)
    return 0;

  unsigned long long value = 0;
  if(fscanf(f, "%llu", &value) != 1)
    value = 0;

  fclose(f);
  return uint64_t(value);
}
}    // namespace

QCCounters::QCCounters()
{
}

QCCounters::~QCCounters()
{
  if(m_Perf.module)
  {
    if(m_Monitor && m_Perf.delete_monitors)
    {
      PFN_glDeletePerfMonitorsAMD del =
          (PFN_glDeletePerfMonitorsAMD)m_Perf.delete_monitors;
      del(1, &m_Monitor);
      m_Monitor = 0;
    }

    dlclose(m_Perf.module);
    m_Perf = {};
  }

  DestroyGLContext();
}

bool QCCounters::CreateGLContext()
{
  // dlopen rather than link: the counter plumbing is compiled into both the GL and the Vulkan
  // driver and must not add a link-time dependency on EGL for the Vulkan one.
  m_EglModule = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
  if(m_EglModule == NULL)
    return false;

  typedef void *(*PFN_eglGetDisplay)(void *);
  typedef int (*PFN_eglInitialize)(void *, int *, int *);
  typedef int (*PFN_eglChooseConfig)(void *, const int32_t *, void **, int32_t, int32_t *);
  typedef void *(*PFN_eglCreateContext)(void *, void *, void *, const int32_t *);
  typedef void *(*PFN_eglCreatePbufferSurface)(void *, void *, const int32_t *);
  typedef int (*PFN_eglMakeCurrent)(void *, void *, void *, void *);
  typedef int (*PFN_eglBindAPI)(uint32_t);
  typedef int (*PFN_eglGetError)();

  PFN_eglGetDisplay getDisplay = (PFN_eglGetDisplay)dlsym(m_EglModule, "eglGetDisplay");
  PFN_eglInitialize initialize = (PFN_eglInitialize)dlsym(m_EglModule, "eglInitialize");
  PFN_eglChooseConfig chooseConfig = (PFN_eglChooseConfig)dlsym(m_EglModule, "eglChooseConfig");
  PFN_eglCreateContext createContext =
      (PFN_eglCreateContext)dlsym(m_EglModule, "eglCreateContext");
  PFN_eglCreatePbufferSurface createSurface =
      (PFN_eglCreatePbufferSurface)dlsym(m_EglModule, "eglCreatePbufferSurface");
  PFN_eglMakeCurrent makeCurrent = (PFN_eglMakeCurrent)dlsym(m_EglModule, "eglMakeCurrent");
  PFN_eglBindAPI bindAPI = (PFN_eglBindAPI)dlsym(m_EglModule, "eglBindAPI");

  if(!getDisplay || !initialize || !chooseConfig || !createContext || !createSurface ||
     !makeCurrent || !bindAPI)
  {
    RDCLOG("QCCounters: libEGL.so is missing the entry points needed to create a context");
    DestroyGLContext();
    return false;
  }

  // EGL_DEFAULT_DISPLAY is a null native display and EGL_NONE is 0.
  m_EglDisplay = getDisplay(NULL);
  if(m_EglDisplay == NULL)
  {
    DestroyGLContext();
    return false;
  }

  int major = 0, minor = 0;
  if(!initialize(m_EglDisplay, &major, &minor))
  {
    RDCLOG("QCCounters: eglInitialize failed for the counter-sampling context");
    m_EglDisplay = NULL;
    DestroyGLContext();
    return false;
  }

  // EGL_OPENGL_ES_API, EGL_OPENGL_ES3_BIT_KHR
  bindAPI(0x30A0);

  const int32_t configAttribs[] = {
      0x3033, 0x0001,    // EGL_SURFACE_TYPE, EGL_PBUFFER_BIT
      0x3040, 0x0004,    // EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT
      0x3024, 8,         // EGL_RED_SIZE
      0x3023, 8,         // EGL_GREEN_SIZE
      0x3022, 8,         // EGL_BLUE_SIZE
      0x3021, 8,         // EGL_ALPHA_SIZE
      0x3038, 0,         // EGL_NONE
  };

  void *config = NULL;
  int32_t numConfigs = 0;
  if(!chooseConfig(m_EglDisplay, configAttribs, &config, 1, &numConfigs) || numConfigs < 1)
  {
    RDCLOG("QCCounters: no EGL config available for the counter-sampling context");
    DestroyGLContext();
    return false;
  }

  const int32_t surfaceAttribs[] = {0x3057, 1, 0x3056, 1, 0x3038, 0};    // WIDTH, HEIGHT, EGL_NONE
  m_EglSurface = createSurface(m_EglDisplay, config, surfaceAttribs);
  if(m_EglSurface == NULL)
  {
    RDCLOG("QCCounters: could not create the counter-sampling pbuffer");
    DestroyGLContext();
    return false;
  }

  // ES 2 is enough for the perf-monitor extension, and asking for the lowest version the driver
  // supports avoids depending on an ES 3 context being creatable next to a Vulkan device.
  const int32_t contextAttribs[] = {0x3098, 2, 0x3038, 0};    // EGL_CONTEXT_CLIENT_VERSION, EGL_NONE
  m_EglContext = createContext(m_EglDisplay, config, NULL, contextAttribs);

  if(m_EglContext == NULL)
  {
    const int32_t es3Attribs[] = {0x3098, 3, 0x3038, 0};
    m_EglContext = createContext(m_EglDisplay, config, NULL, es3Attribs);
  }

  if(m_EglContext == NULL)
  {
    RDCLOG("QCCounters: could not create the counter-sampling GLES context");
    DestroyGLContext();
    return false;
  }

  if(!makeCurrent(m_EglDisplay, m_EglSurface, m_EglSurface, m_EglContext))
  {
    RDCLOG("QCCounters: could not make the counter-sampling context current");
    DestroyGLContext();
    return false;
  }

  m_OwnContext = true;
  RDCLOG("QCCounters: created a private GLES context to sample counters from the Vulkan replay");
  return true;
}

void QCCounters::DestroyGLContext()
{
  if(m_EglModule == NULL)
    return;

  typedef int (*PFN_eglMakeCurrent)(void *, void *, void *, void *);
  typedef int (*PFN_eglDestroyContext)(void *, void *);
  typedef int (*PFN_eglDestroySurface)(void *, void *);
  typedef int (*PFN_eglTerminate)(void *);

  PFN_eglMakeCurrent makeCurrent = (PFN_eglMakeCurrent)dlsym(m_EglModule, "eglMakeCurrent");
  PFN_eglDestroyContext destroyContext =
      (PFN_eglDestroyContext)dlsym(m_EglModule, "eglDestroyContext");
  PFN_eglDestroySurface destroySurface =
      (PFN_eglDestroySurface)dlsym(m_EglModule, "eglDestroySurface");
  PFN_eglTerminate terminate = (PFN_eglTerminate)dlsym(m_EglModule, "eglTerminate");

  if(makeCurrent && m_EglDisplay)
    makeCurrent(m_EglDisplay, NULL, NULL, NULL);

  if(destroyContext && m_EglDisplay && m_EglContext)
    destroyContext(m_EglDisplay, m_EglContext);

  if(destroySurface && m_EglDisplay && m_EglSurface)
    destroySurface(m_EglDisplay, m_EglSurface);

  if(terminate && m_EglDisplay)
    terminate(m_EglDisplay);

  m_EglDisplay = NULL;
  m_EglContext = NULL;
  m_EglSurface = NULL;
  m_OwnContext = false;

  dlclose(m_EglModule);
  m_EglModule = NULL;
}

void QCCounters::MakeCurrent()
{
  if(!m_OwnContext || m_EglDisplay == NULL)
    return;

  typedef int (*PFN_eglMakeCurrent)(void *, void *, void *, void *);
  PFN_eglMakeCurrent makeCurrent = (PFN_eglMakeCurrent)dlsym(m_EglModule, "eglMakeCurrent");

  if(makeCurrent)
    makeCurrent(m_EglDisplay, m_EglSurface, m_EglSurface, m_EglContext);
}

bool QCCounters::Init(ApiType api, void *context, const char *renderer)
{
  m_Api = api;
  m_ApiContext = context;

  // The counters are read through the GL driver's perf-monitor extension. There is no counter
  // extension on this driver's Vulkan side, but the counters are hardware block activity rather
  // than anything owned by a particular API or context, so a Vulkan replay samples them through a
  // private GLES context created inside the replay process.
  if(api == ApiType::Vk && !CreateGLContext())
  {
    RDCLOG("QCCounters: could not create the GLES context a Vulkan replay needs to sample "
           "counters with");
    return false;
  }

  if(!LoadPerfMon())
  {
    RDCLOG("QCCounters: this driver does not expose GL_AMD_performance_monitor entry points");
    return false;
  }

  m_AdrenoPart = QCGGPMDeviceIdFromRenderer(renderer);
  if(m_AdrenoPart == 0)
    RDCLOG("QCCounters: unrecognised GPU \"%s\", metrics that scale by pipe counts will be "
           "reported as unavailable",
           renderer ? renderer : "");

  SampleDeviceClock();

  EnumerateCatalog();
  if(m_DeviceGroups.empty())
  {
    RDCLOG("QCCounters: the driver reported no performance monitor groups");
    return false;
  }

  ResolveNeeds();
  BuildCatalog();

  RDCLOG("QCCounters: %u Adreno groups, %u raw counters resolved of %u needed, %u metrics "
         "published (max clock %.0f Hz)",
         (uint32_t)m_DeviceGroups.size(),
         (uint32_t)std::count_if(m_Needs.begin(), m_Needs.end(),
                                 [](const NeedBinding &n) { return n.resolved; }),
         kQCGGPMNeedCount, (uint32_t)m_Built.size(), m_MaxClockHz);

  return !m_Built.empty();
}

bool QCCounters::LoadPerfMon()
{
  // The extension string cannot be trusted: this Adreno 610 reports zero
  // GL_AMD_performance_monitor counters in the extension list while the entry points below are
  // all present and sample real data. Availability is therefore decided by the entry points.
  void *lib = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
  if(lib == NULL)
    return false;

  typedef void *(*PFN_eglGetProcAddress)(const char *);
  PFN_eglGetProcAddress getProc =
      (PFN_eglGetProcAddress)dlsym(lib, "eglGetProcAddress");
  if(getProc == NULL)
  {
    dlclose(lib);
    return false;
  }

  struct Entry
  {
    void **slot;
    const char *name;
  };

  const Entry entries[] = {
      {(void **)&m_Perf.gen_monitors, "glGenPerfMonitorsAMD"},
      {(void **)&m_Perf.delete_monitors, "glDeletePerfMonitorsAMD"},
      {(void **)&m_Perf.get_groups, "glGetPerfMonitorGroupsAMD"},
      {(void **)&m_Perf.get_counters, "glGetPerfMonitorCountersAMD"},
      {(void **)&m_Perf.get_group_string, "glGetPerfMonitorGroupStringAMD"},
      {(void **)&m_Perf.get_counter_string, "glGetPerfMonitorCounterStringAMD"},
      {(void **)&m_Perf.get_counter_info, "glGetPerfMonitorCounterInfoAMD"},
      {(void **)&m_Perf.select_counters, "glSelectPerfMonitorCountersAMD"},
      {(void **)&m_Perf.begin, "glBeginPerfMonitorAMD"},
      {(void **)&m_Perf.end, "glEndPerfMonitorAMD"},
      {(void **)&m_Perf.get_counter_data, "glGetPerfMonitorCounterDataAMD"},
      // Not fatal if missing: only used to push the monitor's start marker down to the GPU
      // before unrelated work is submitted, which matters when sampling a Vulkan replay.
      {(void **)&m_Perf.flush, "glFlush"},
  };

  bool complete = true;
  for(const Entry &e : entries)
  {
    *e.slot = getProc(e.name);

    if(*e.slot == NULL && strcmp(e.name, "glFlush") == 0)
      continue;

    if(*e.slot == NULL)
    {
      RDCLOG("QCCounters: %s is missing", e.name);
      complete = false;
    }
  }

  if(!complete)
  {
    dlclose(lib);
    m_Perf = {};
    return false;
  }

  m_Perf.module = lib;
  return true;
}

void QCCounters::EnumerateCatalog()
{
  PFN_glGetPerfMonitorGroupsAMD getGroups = (PFN_glGetPerfMonitorGroupsAMD)m_Perf.get_groups;
  PFN_glGetPerfMonitorCountersAMD getCounters =
      (PFN_glGetPerfMonitorCountersAMD)m_Perf.get_counters;
  PFN_glGetPerfMonitorGroupStringAMD getGroupString =
      (PFN_glGetPerfMonitorGroupStringAMD)m_Perf.get_group_string;
  PFN_glGetPerfMonitorCounterStringAMD getCounterString =
      (PFN_glGetPerfMonitorCounterStringAMD)m_Perf.get_counter_string;

  int32_t groupCount = 0;
  getGroups(&groupCount, 0, NULL);

  if(groupCount <= 0)
    return;

  rdcarray<uint32_t> groupIds;
  groupIds.resize(groupCount);
  int32_t written = groupCount;
  getGroups(&written, groupCount, groupIds.data());

  for(int32_t g = 0; g < written; g++)
  {
    DeviceGroup group;
    group.group = groupIds[g];

    int32_t length = 0;
    getGroupString(group.group, 0, &length, NULL);
    group.name.resize(length + 1);
    getGroupString(group.group, length + 1, &length, group.name.data());
    group.name.resize(length + 1);
    group.name.back() = 0;

    int32_t counterCount = 0;
    int32_t maxActive = 0;
    getCounters(group.group, &counterCount, &maxActive, 0, NULL);
    group.maxActive = maxActive > 0 ? uint32_t(maxActive) : 1;

    if(counterCount > 0)
    {
      rdcarray<uint32_t> counterIds;
      counterIds.resize(counterCount);
      int32_t counterWritten = counterCount;
      getCounters(group.group, &counterWritten, &maxActive, counterCount, counterIds.data());

      for(int32_t c = 0; c < counterWritten; c++)
      {
        DeviceCounter counter;
        counter.group = group.group;
        counter.counter = counterIds[c];

        int32_t nameLength = 0;
        getCounterString(group.group, counter.counter, 0, &nameLength, NULL);
        counter.name.resize(nameLength + 1);
        getCounterString(group.group, counter.counter, nameLength + 1, &nameLength,
                         counter.name.data());
        counter.name.resize(nameLength + 1);
        counter.name.back() = 0;

        group.counters.push_back(counter);
      }
    }

    m_DeviceGroups.push_back(group);
  }
}

const QCCounters::DeviceCounter *QCCounters::FindDeviceCounter(const rdcarray<DeviceGroup> &groups,
                                                              const char *name, uint32_t *maxActive)
{
  for(const DeviceGroup &group : groups)
  {
    for(const DeviceCounter &counter : group.counters)
    {
      if(NameEquals(counter.name.data(), name))
      {
        *maxActive = group.maxActive;
        return &counter;
      }
    }
  }

  return NULL;
}

void QCCounters::ResolveNeeds()
{
  m_Needs.clear();

  uint32_t unresolved = 0;

  for(uint32_t i = 0; i < kQCGGPMNeedCount; i++)
  {
    NeedBinding binding;
    binding.index = i;

    const char *name = QCGGPMCounterName(kQCGGPMNeeds[i].block, kQCGGPMNeeds[i].id);
    if(name == NULL)
    {
      RDCWARN("QCCounters: no profiler name for %s[%u]", kQCGGPMNeeds[i].block,
              kQCGGPMNeeds[i].id);
      unresolved++;
      m_Needs.push_back(binding);
      continue;
    }

    uint32_t maxActive = 1;
    const DeviceCounter *counter = FindDeviceCounter(m_DeviceGroups, name, &maxActive);
    if(counter != NULL)
    {
      binding.group = counter->group;
      binding.counter = counter->counter;
      binding.maxActive = maxActive;
      binding.resolved = true;
    }
    else
    {
      RDCLOG("QCCounters: this driver does not report %s[%u] (\"%s\")", kQCGGPMNeeds[i].block,
             kQCGGPMNeeds[i].id, name);
      unresolved++;
    }

    m_Needs.push_back(binding);
  }

  if(unresolved > 0)
    RDCLOG("QCCounters: %u of %u raw counters are unavailable on this device", unresolved,
           kQCGGPMNeedCount);
}

void QCCounters::SampleDeviceClock()
{
  // The profiler's "% Utilization" divides by the part's maximum GPU clock, not by the clock the
  // GPU happens to be running at, so that is the value that has to be read here. The kgsl node is
  // readable on a rooted device; when it is not, that metric reports unavailable rather than
  // dividing by zero.
  m_MaxClockHz = double(ReadSysUInt64("/sys/class/kgsl/kgsl-3d0/max_gpuclk"));

  if(m_MaxClockHz <= 0.0)
    RDCLOG("QCCounters: max_gpuclk is not readable, GPU %% Utilization will be unavailable");
}

void QCCounters::BuildCatalog()
{
  m_CounterIds.clear();
  m_Built.clear();

  for(uint32_t i = 0; i < kQCGGPMMetricCount; i++)
  {
    const QCGGPMMetric &metric = kQCGGPMMetrics[i];

    BuiltMetric built;
    built.id = StableIdFromKey(metric.key);
    built.metric = i;

    // A metric is only published when every raw counter it reads is addressable here. Publishing
    // it anyway would offer a counter that can never produce a value, and the user could not tell
    // that apart from a GPU that genuinely has nothing to report.
    built.available = true;

    uint32_t deps = 0;
    for(uint32_t d = 0; d < kQCGGPMDepCount; d++)
    {
      if(strcmp(kQCGGPMDeps[d].metric, metric.key) != 0)
        continue;

      deps++;

      const int needIndex = QCGGPMSample::IndexOf(kQCGGPMDeps[d].block, kQCGGPMDeps[d].id);
      if(needIndex < 0 || !m_Needs[size_t(needIndex)].resolved)
        built.available = false;
    }

    // A metric with no declared dependency would always report a constant, so treat it as
    // unavailable rather than publishing it.
    if(deps == 0)
      built.available = false;

    m_Built.push_back(built);
    m_CounterIds.push_back(built.id);
  }
}

GPUCounter QCCounters::StableIdFromKey(const char *key)
{
  // FNV-1a over the metric key, folded into the Qualcomm counter range. Deriving the public ID
  // from the name rather than from the list position guarantees a given counter keeps the same
  // GPUCounter value on every Adreno generation and across versions, even though the set of
  // metrics a device can evaluate differs.
  const uint32_t range =
      uint32_t(GPUCounter::LastQualcomm) - uint32_t(GPUCounter::FirstQualcomm) + 1;

  uint32_t hash = 2166136261u;
  for(const char *p = key; p != NULL && *p; p++)
  {
    hash ^= uint8_t(*p);
    hash *= 16777619u;
  }

  return GPUCounter(uint32_t(GPUCounter::FirstQualcomm) + (hash % range));
}

int32_t QCCounters::IndexForId(GPUCounter counter) const
{
  for(uint32_t i = 0; i < m_Built.size(); i++)
  {
    if(m_Built[i].id == counter)
      return int32_t(i);
  }

  return -1;
}

rdcarray<GPUCounter> QCCounters::GetPublicCounterIds()
{
  rdcarray<GPUCounter> ret;
  for(const BuiltMetric &built : m_Built)
  {
    if(built.available)
      ret.push_back(built.id);
  }

  return ret;
}

CounterDescription QCCounters::GetCounterDescription(GPUCounter counter)
{
  const int32_t index = IndexForId(counter);

  CounterDescription desc;
  desc.counter = counter;

  if(index < 0)
  {
    desc.name = "Unknown";
    return desc;
  }

  const QCGGPMMetric &metric = kQCGGPMMetrics[m_Built[size_t(index)].metric];

  desc.name = metric.label;
  desc.description = metric.description;
  desc.category = metric.category;
  desc.resultType = CompType::Float;
  desc.resultByteWidth = 8;

  // The units map onto RenderDoc's counter units so the table shows the right suffix. The values
  // themselves are in the same scale Snapdragon Profiler reports, i.e. percentages are 0..100 and
  // "% Texture L2 Miss" can legitimately exceed 100.
  if(strcmp(metric.unit, "%") == 0)
    desc.unit = CounterUnit::Percentage;
  else if(strcmp(metric.unit, "B") == 0 || strcmp(metric.unit, "B/s") == 0)
    desc.unit = CounterUnit::Bytes;
  else if(strcmp(metric.unit, "cyc") == 0)
    desc.unit = CounterUnit::Cycles;
  else if(strcmp(metric.unit, "us") == 0)
    desc.unit = CounterUnit::Absolute;    // microseconds: RenderDoc has no microsecond unit, and
                                         // calling it Seconds would display values off by 1e6
  else
    desc.unit = CounterUnit::Absolute;

  return desc;
}

void QCCounters::DisableAllCounters()
{
  m_EnabledCounters.clear();
  m_Passes.clear();

  // Each fetch starts from a clean slate: a sample left over from an earlier fetch would be
  // paired with counters it was never measured for. The session-wide cache is deliberately kept:
  // it only ever holds values that were really measured, and ReuseSample() checks each counter a
  // selection reads before handing a cached event back.
  m_Samples.clear();
}

void QCCounters::EnableCounter(GPUCounter counter)
{
  const int32_t index = IndexForId(counter);
  if(index < 0)
    return;

  if(!m_EnabledCounters.contains(uint32_t(index)))
    m_EnabledCounters.push_back(uint32_t(index));

  // The pass layout depends on which metrics are enabled, since it is built from the raw counters
  // those metrics read.
  m_Passes.clear();
}

void QCCounters::BuildPasses()
{
  m_Passes.clear();

  if(m_EnabledCounters.empty())
    return;

  // Which raw counters are actually needed: the union, over the enabled metrics, of the counters
  // their formulas read (kQCGGPMDeps). Selecting only those keeps a fetch from spending GPU time
  // sampling blocks no enabled metric looks at.
  rdcarray<uint32_t> needed;

  for(uint32_t enabled : m_EnabledCounters)
  {
    const char *key = kQCGGPMMetrics[m_Built[enabled].metric].key;

    for(uint32_t d = 0; d < kQCGGPMDepCount; d++)
    {
      if(strcmp(kQCGGPMDeps[d].metric, key) != 0)
        continue;

      const int needIndex = QCGGPMSample::IndexOf(kQCGGPMDeps[d].block, kQCGGPMDeps[d].id);
      if(needIndex < 0 || !m_Needs[size_t(needIndex)].resolved)
        continue;

      if(!needed.contains(uint32_t(needIndex)))
        needed.push_back(uint32_t(needIndex));
    }
  }

  if(needed.empty())
    return;

  // One pass can hold at most `maxActive` counters from the same hardware block, so walk the
  // blocks in turn and start a new pass whenever a block's budget would be exceeded.
  std::map<uint32_t, uint32_t> usedInPass;

  rdcarray<uint32_t> current;

  for(uint32_t needIndex : needed)
  {
    const NeedBinding &binding = m_Needs[needIndex];
    const uint32_t limit = binding.maxActive > 0 ? binding.maxActive : 1;

    if(usedInPass[binding.group] + 1 > limit)
    {
      m_Passes.push_back(current);
      current.clear();
      usedInPass.clear();
    }

    usedInPass[binding.group]++;
    current.push_back(needIndex);
  }

  if(!current.empty())
    m_Passes.push_back(current);
}

uint32_t QCCounters::GetPassCount()
{
  if(m_Passes.empty())
    BuildPasses();

  return uint32_t(m_Passes.size());
}

void QCCounters::BeginPass(uint32_t passID)
{
  m_OpenPass = passID;

  MakeCurrent();

  if(passID >= m_Passes.size())
    return;

  // A monitor is created per pass so the selection cannot accumulate across passes: the driver's
  // select call only adds to the current selection.
  PFN_glGenPerfMonitorsAMD gen = (PFN_glGenPerfMonitorsAMD)m_Perf.gen_monitors;
  PFN_glSelectPerfMonitorCountersAMD select =
      (PFN_glSelectPerfMonitorCountersAMD)m_Perf.select_counters;

  if(m_Monitor == 0)
    gen(1, &m_Monitor);

  if(m_Monitor == 0)
    return;

  const rdcarray<uint32_t> &pass = m_Passes[passID];

  // Group the selection by hardware block: one select call per group with all of that block's
  // counters in this pass, which is the form the extension is specified for.
  for(size_t i = 0; i < pass.size();)
  {
    const uint32_t group = m_Needs[pass[i]].group;

    rdcarray<uint32_t> counters;
    while(i < pass.size() && m_Needs[pass[i]].group == group)
    {
      counters.push_back(m_Needs[pass[i]].counter);
      i++;
    }

    select(m_Monitor, 1, group, int32_t(counters.size()), counters.data());
  }
}

void QCCounters::EndPass()
{
  MakeCurrent();

  if(m_Monitor && m_Perf.delete_monitors)
  {
    PFN_glDeletePerfMonitorsAMD del = (PFN_glDeletePerfMonitorsAMD)m_Perf.delete_monitors;
    del(1, &m_Monitor);
    m_Monitor = 0;
  }
}

bool QCCounters::ReuseSample(uint32_t eventId)
{
  std::map<uint32_t, EventSample>::const_iterator it = m_SampleCache.find(eventId);
  if(it == m_SampleCache.end())
    return false;

  // A pass layout of zero passes means the selection reads no raw counter at all, so there is
  // nothing a cached event could answer.
  if(m_Passes.empty())
    return false;

  for(size_t p = 0; p < m_Passes.size(); p++)
  {
    for(size_t i = 0; i < m_Passes[p].size(); i++)
    {
      const uint32_t need = m_Passes[p][i];
      if(need >= it->second.attempted.size() || !it->second.attempted[need])
        return false;
    }
  }

  m_Samples[eventId] = it->second;
  return true;
}

void QCCounters::BeginSample(uint32_t eventId)
{
  if(m_Monitor == 0 || m_Sampling)
    return;

  MakeCurrent();

  PFN_glBeginPerfMonitorAMD begin = (PFN_glBeginPerfMonitorAMD)m_Perf.begin;
  begin(m_Monitor);

  // Push the start marker to the GPU now. When the work being measured is submitted from another
  // API (a Vulkan replay) nothing else would flush this context before that work runs, and the
  // window would then start after it.
  if(m_Perf.flush)
  {
    typedef void (*PFN_glFlush)();
    ((PFN_glFlush)m_Perf.flush)();
  }

  m_CurrentEvent = eventId;
  m_SampleStartSec = NowSeconds();
  m_Sampling = true;
}

void QCCounters::EndSample()
{
  if(!m_Sampling)
    return;

  m_Sampling = false;

  MakeCurrent();

  PFN_glEndPerfMonitorAMD end = (PFN_glEndPerfMonitorAMD)m_Perf.end;
  end(m_Monitor);

  // The caller has already waited on a fence, so the GPU has finished this draw and the result
  // should be ready. The wait below is a bounded safety net for the case where the driver defers
  // publication; when it expires the sample is dropped rather than filled with stale values.
  PFN_glGetPerfMonitorCounterDataAMD getData =
      (PFN_glGetPerfMonitorCounterDataAMD)m_Perf.get_counter_data;

  uint32_t available = 0;
  const double waitEnd = NowSeconds() + 2.0;
  while(NowSeconds() < waitEnd)
  {
    getData(m_Monitor, GL_PERFMON_RESULT_AVAILABLE_AMD, sizeof(available), &available, NULL);
    if(available)
      break;
    usleep(1000);
  }

  if(!available)
  {
    RDCWARN("QCCounters: no result for event %u, counters for this draw are unavailable",
            m_CurrentEvent);
    return;
  }

  const double window = NowSeconds() - m_SampleStartSec;

  int32_t size = 0;
  getData(m_Monitor, GL_PERFMON_RESULT_SIZE_AMD, sizeof(size), (uint32_t *)&size, NULL);

  if(size <= 0)
    return;

  // The driver reports one record per selected counter. Terminating zeroes are not one of the
  // layouts the extension defines, so the buffer is read with the size the driver just reported.
  rdcarray<uint32_t> words;
  words.resize(size_t(size / 4) + 1);

  int32_t written = 0;
  getData(m_Monitor, GL_PERFMON_RESULT_AMD, size, words.data(), &written);

  EventSample &sample = m_Samples[m_CurrentEvent];

  // A sample accumulates across passes: each pass selects a different slice of the raw counters, so
  // resetting here would erase the values an earlier pass already collected for this event.
  if(sample.raw.size() != size_t(kQCGGPMNeedCount))
  {
    sample.raw.resize(kQCGGPMNeedCount);
    for(uint32_t i = 0; i < kQCGGPMNeedCount; i++)
      sample.raw[i] = -1.0;
  }

  sample.dt = window;

  const size_t wordCount = written > 0 ? size_t(written / 4) : size_t(size / 4);

  if(m_Passes.empty())
    return;

  ReadResults(m_Passes[m_OpenPass < m_Passes.size() ? m_OpenPass : 0], words.data(), wordCount,
              sample.raw.data());

  // Fold this pass into the session cache. Values are only ever merged, never cleared, so a pass
  // that covers a different slice of the raw counters adds to what earlier passes measured for the
  // same event instead of replacing it.
  EventSample &cached = m_SampleCache[m_CurrentEvent];
  if(cached.raw.size() != size_t(kQCGGPMNeedCount))
  {
    cached.raw.resize(kQCGGPMNeedCount);
    for(uint32_t i = 0; i < kQCGGPMNeedCount; i++)
      cached.raw[i] = -1.0;
  }
  if(cached.attempted.size() != size_t(kQCGGPMNeedCount))
  {
    cached.attempted.resize(kQCGGPMNeedCount);
    for(uint32_t i = 0; i < kQCGGPMNeedCount; i++)
      cached.attempted[i] = 0;
  }
  const rdcarray<uint32_t> &openPass =
      m_Passes[m_OpenPass < m_Passes.size() ? m_OpenPass : 0];
  for(size_t i = 0; i < openPass.size(); i++)
  {
    if(openPass[i] < uint32_t(cached.attempted.size()))
      cached.attempted[openPass[i]] = 1;
  }
  if(cached.dt <= 0.0)
    cached.dt = window;
  for(size_t i = 0; i < sample.raw.size() && i < cached.raw.size(); i++)
  {
    if(sample.raw[i] >= 0.0)
      cached.raw[i] = sample.raw[i];
  }
}

bool QCCounters::ReadResults(const rdcarray<uint32_t> &needIndices, const uint32_t *words,
                             size_t wordCount, double *out)
{
  // Two result layouts exist in the wild and they cannot be told apart by the extension string:
  //
  //   * the Adreno driver on this part emits a fixed four-word record per counter,
  //     {group, counter, value, unused}. It is NOT the layout the extension specifies.
  //   * the specified layout is {group, counter, numInstances, type} followed by numInstances
  //     values.
  //
  // Both layouts carry the group and counter of every record, so the values are read by matching
  // those against the selection instead of by position. A layout whose records do not map onto
  // the selection is rejected outright rather than producing plausible-looking garbage.
  struct Wanted
  {
    uint32_t group;
    uint32_t counter;
    uint32_t need;
  };

  rdcarray<Wanted> wanted;
  wanted.reserve(needIndices.size());
  for(uint32_t needIndex : needIndices)
  {
    Wanted w;
    w.group = m_Needs[needIndex].group;
    w.counter = m_Needs[needIndex].counter;
    w.need = needIndex;
    wanted.push_back(w);
  }

  bool any = false;

  // Layout 1: 4 words per record {group, counter, value, unused}.
  //
  // The count is not required to match the selection exactly - this driver drops a few records
  // when a block hits its active-counter limit - but every record that is present must map onto a
  // selected counter and must carry the zero fourth word, which is what separates this layout from
  // the specified one below.
  if((wordCount % 4) == 0 && wordCount >= 4)
  {
    bool matches = true;

    for(size_t i = 0; i < wordCount / 4 && matches; i++)
    {
      if(words[i * 4 + 3] != 0)
      {
        matches = false;
        break;
      }

      bool found = false;
      for(const Wanted &w : wanted)
      {
        if(w.group == words[i * 4 + 0] && w.counter == words[i * 4 + 1])
        {
          found = true;
          break;
        }
      }

      if(!found)
        matches = false;
    }

    if(matches)
    {
      for(size_t i = 0; i < wordCount / 4; i++)
      {
        for(const Wanted &w : wanted)
        {
          if(w.group == words[i * 4 + 0] && w.counter == words[i * 4 + 1])
          {
            out[w.need] = double(words[i * 4 + 2]);
            any = true;
            break;
          }
        }
      }

      return any;
    }
  }

  // Layout 2: the layout the extension actually specifies.
  size_t offset = 0;
  while(offset + 4 <= wordCount)
  {
    const uint32_t group = words[offset + 0];
    const uint32_t counter = words[offset + 1];
    const uint32_t instances = words[offset + 2];
    const uint32_t type = words[offset + 3];

    offset += 4;

    if(instances == 0 || offset + instances > wordCount)
      break;

    // Guard against reading a buffer in the wrong layout: the specified layout always records a
    // GL enum for the type, and a value-shaped word would not be one.
    if(type != GL_FLOAT && type != GL_UNSIGNED_INT && type != GL_UNSIGNED_BYTE && type != GL_INT)
      break;

    for(const Wanted &w : wanted)
    {
      if(w.group == group && w.counter == counter)
      {
        // Only the first instance is used: every counter selected here is a scalar block counter.
        double value = 0.0;
        if(type == GL_FLOAT)
          memcpy(&value, &words[offset], sizeof(float));
        else
          value = double(words[offset]);

        out[w.need] = value;
        any = true;
        break;
      }
    }

    offset += instances;
  }

  return any;
}

rdcarray<CounterResult> QCCounters::GetCounterData(const rdcarray<uint32_t> &eventIDs,
                                                   const rdcarray<GPUCounter> &counters)
{
  rdcarray<CounterResult> ret;

  for(uint32_t eventId : eventIDs)
  {
    std::map<uint32_t, EventSample>::const_iterator it = m_Samples.find(eventId);
    if(it == m_Samples.end())
      continue;

    QCGGPMSample sample;
    sample.Init();
    sample.dt = it->second.dt;
    sample.maxClockHz = m_MaxClockHz;
    sample.deviceId = m_AdrenoPart;

    const size_t count = std::min(it->second.raw.size(), size_t(kQCGGPMNeedCount));
    for(size_t i = 0; i < count; i++)
    {
      sample.values[i] = it->second.raw[i];
      if(sample.values[i] >= 0.0)
        sample.countersReady = true;
    }

    for(GPUCounter counter : counters)
    {
      const int32_t index = IndexForId(counter);
      if(index < 0)
        continue;

      const QCGGPMMetric &metric = kQCGGPMMetrics[m_Built[size_t(index)].metric];

      double value = 0.0;
      if(!QCGGPMEvaluate(metric.key, sample, &value))
        continue;

      ret.push_back(CounterResult(eventId, counter, value));
    }
  }

  return ret;
}

#endif    // ENABLED(RDOC_ANDROID)

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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>

#include "qc_ggpm_metrics.h"
#include "qc_ggpm_plan.h"

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
  GL_UNSIGNED_INT64_AMD = 0x8BC2,
  GL_PERCENTAGE_AMD = 0x8BC3,
  GL_NO_ERROR = 0,
  // Qualcomm's enable that makes the perf-monitor counters count GPU-wide activity and publish
  // results. It is not in the extension string this driver reports, but glEnable accepts it.
  GL_PERFMON_GLOBAL_MODE_QCOM = 0x8FA0,
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
typedef uint32_t (*PFN_glGetError)();

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

// The sampling sequence can only be observed through the device log, and a per-event line would
// flood it, so the detailed trace is opt-in: /data/local/tmp/qc_debug on the device, or
// RENDERDOC_QC_DEBUG in the replay process' environment.
bool QCDebugLogEnabled()
{
  static int cached = -1;

  if(cached < 0)
  {
    const char *env = getenv("RENDERDOC_QC_DEBUG");
    cached = (env != NULL && env[0] != '\0') || access("/data/local/tmp/qc_debug", F_OK) == 0;
  }

  return cached != 0;
}

// Read one line-terminated search over /proc/self/maps. Used to tell whether another profiling
// layer was injected into this process, which no GL API can report.
bool QCMapsContain(const char *needle)
{
  FILE *f = fopen("/proc/self/maps", "r");
  if(f == NULL)
    return false;

  char line[512];
  bool found = false;
  while(!found && fgets(line, sizeof(line), f) != NULL)
    found = strstr(line, needle) != NULL;

  fclose(f);
  return found;
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
  // Checked before any GL work, including creating the private context: Snapdragon Profiler's Q3D
  // tools layer is injected into every EGL process while debug.egl.profiler is 1, and it interposes
  // the same perf-monitor entry points this sampling uses. Sampling under it has been seen to fault
  // inside libGLESv2_adreno.so rather than merely fail, so a process carrying it is refused instead
  // of being allowed to crash the replay. RENDERDOC_QC_ALLOW_Q3D=1 overrides this for experiments.
  if(QCMapsContain("libq3dtools") && getenv("RENDERDOC_QC_ALLOW_Q3D") == NULL)
  {
    RDCWARN("QCCounters: Snapdragon Profiler's Q3D tools layer (libq3dtools) is loaded in this "
            "process, so Adreno counters are disabled here: it interposes the perf-monitor entry "
            "points and sampling under it has faulted inside the GL driver. Stop the profiler and "
            "set debug.egl.profiler to 0 (adb shell su -c 'setprop debug.egl.profiler 0'; adb "
            "shell su -c 'pkill -f sdpservice; pkill -f pluginGPU; pkill -f SnapshotPlayer'), then "
            "start a new session. RENDERDOC_QC_ALLOW_Q3D=1 skips this check.");
    return false;
  }

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

  // The planning and decoding arithmetic is shared with the desktop build, so it is exercised there
  // on every build; running it here too means a plan that would silently drop counters is reported
  // on the device that would have lost them.
  {
    char planError[256] = {};
    const int planFailures = QCGGPMPlanSelfCheck(m_AdrenoPart, planError, sizeof(planError));
    if(planFailures != 0)
      RDCWARN("QCCounters: %d planning/decode self-check failure(s), first: %s", planFailures,
              planError);
  }

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
      // Not fatal if missing: without it a refused selection cannot be told apart from a selection
      // that simply produced no records, and the whole block is lost instead of one counter.
      {(void **)&m_Perf.get_error, "glGetError"},
      // Not fatal if missing on its own, but a monitor cannot be sampled without the global mode
      // enable on Adreno, so LoadPerfMon() treats a missing pair as fatal below.
      {(void **)&m_Perf.gl_enable, "glEnable"},
      {(void **)&m_Perf.gl_disable, "glDisable"},
      {(void **)&m_Perf.gl_finish, "glFinish"},
  };

  bool complete = true;
  for(const Entry &e : entries)
  {
    *e.slot = getProc(e.name);

    // eglGetProcAddress only answers extension entry points on some drivers, and glEnable/glDisable
    // are core GL, so fall back to the process' own symbol table before calling a slot missing.
    if(*e.slot == NULL)
      *e.slot = dlsym(RTLD_DEFAULT, e.name);

    if(*e.slot == NULL && strcmp(e.name, "glFlush") == 0)
      continue;

    if(*e.slot == NULL && strcmp(e.name, "glGetError") == 0)
      continue;

    if(*e.slot == NULL && (strcmp(e.name, "glEnable") == 0 || strcmp(e.name, "glDisable") == 0))
      continue;

    if(*e.slot == NULL && strcmp(e.name, "glFinish") == 0)
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

  // Without these two the global mode enable cannot be turned on, and a monitor sampled without it
  // makes the driver dereference a result buffer it never allocated, so the counters are reported
  // as unavailable rather than sampled and crashing the replay.
  if(m_Perf.gl_enable == NULL || m_Perf.gl_disable == NULL)
  {
    RDCLOG("QCCounters: glEnable/glDisable are not reachable, so the perf monitor global mode "
           "cannot be enabled and the counters cannot be sampled");
    dlclose(lib);
    m_Perf = {};
    return false;
  }

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

  PFN_glGetPerfMonitorCounterInfoAMD getCounterInfo =
      (PFN_glGetPerfMonitorCounterInfoAMD)m_Perf.get_counter_info;

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
        getCounterInfo(group.group, counter.counter, GL_COUNTER_TYPE_AMD, &counter.type);

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
  m_Refused.fill(size_t(kQCGGPMNeedCount), 0);

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
      binding.type = counter->type;
      binding.resolved = binding.type == GL_UNSIGNED_INT || binding.type == GL_UNSIGNED_INT64_AMD ||
                         binding.type == GL_FLOAT || binding.type == GL_PERCENTAGE_AMD;
      if(!binding.resolved)
        unresolved++;
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

// The indices in kQCGGPMNeeds of every raw counter a metric's formula reads on this part. The
// resolution itself lives in qc_ggpm_plan.h so the desktop build compiles and runs the same code;
// this adapter only converts the result into the container this file uses.
static rdcarray<int> MetricNeeds(const char *key, uint32_t deviceId)
{
  std::vector<int> needs;
  QCGGPMMetricNeeds(key, deviceId, needs);

  rdcarray<int> ret;
  ret.reserve(needs.size());
  for(size_t i = 0; i < needs.size(); i++)
    ret.push_back(needs[i]);
  return ret;
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

    const rdcarray<int> needs = MetricNeeds(metric.key, m_AdrenoPart);

    for(int needIndex : needs)
    {
      if(needIndex < 0 || !m_Needs[size_t(needIndex)].resolved)
      {
        built.available = false;
        break;
      }
    }

    // A metric with no declared dependency would always report a constant, so treat it as
    // unavailable rather than publishing it.
    if(needs.empty())
      built.available = false;

    m_Built.push_back(built);
    m_CounterIds.push_back(built.id);
  }
}

GPUCounter QCCounters::StableIdFromKey(const char *key)
{
  // The mapping lives in qc_ggpm_plan.h: the same hash has to be produced by the device build, the
  // desktop build and the recovered-profile checks, so there is one implementation of it.
  return GPUCounter(QCGGPMStableId(key, uint32_t(GPUCounter::FirstQualcomm),
                                   uint32_t(GPUCounter::LastQualcomm)));
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

  ReleaseMonitors();
  SetGlobalMode(false);

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

// Spreads the counters over several passes, one per block budget, unless the device is known to
// take them all at once. On by /data/local/tmp/qc_multipass or RENDERDOC_QC_MULTIPASS.
static bool MultiPassEnabled()
{
  static int cached = -1;

  if(cached < 0)
  {
    const char *env = getenv("RENDERDOC_QC_MULTIPASS");
    cached = (env != NULL && env[0] != '0') ? 1 : 0;

    if(cached == 0)
    {
      FILE *f = fopen("/data/local/tmp/qc_multipass", "rb");
      if(f != NULL)
      {
        cached = 1;
        fclose(f);
      }
    }
  }

  return cached == 1;
}

void QCCounters::CollectNeeds(rdcarray<uint32_t> &needed, rdcarray<uint32_t> &dedicated)
{
  needed.clear();
  dedicated.clear();

  // Which raw counters are actually needed: the union, over the enabled metrics, of the counters
  // their formulas read (kQCGGPMDeps). Selecting only those keeps a fetch from spending GPU time
  // sampling blocks no enabled metric looks at.
  for(uint32_t enabled : m_EnabledCounters)
  {
    const char *key = kQCGGPMMetrics[m_Built[enabled].metric].key;

    for(int needIndex : MetricNeeds(key, m_AdrenoPart))
    {
      if(needIndex < 0 || !m_Needs[size_t(needIndex)].resolved)
        continue;

      // A counter the driver would not select alongside the others still has to be measured, since
      // its metrics are only unavailable because of the combination. It gets a pass with nothing
      // else in it, where the same select call is known to be accepted.
      if(size_t(needIndex) < m_Refused.size() && m_Refused[size_t(needIndex)])
      {
        if(!dedicated.contains(uint32_t(needIndex)))
          dedicated.push_back(uint32_t(needIndex));
        continue;
      }

      if(!needed.contains(uint32_t(needIndex)))
        needed.push_back(uint32_t(needIndex));
    }
  }
}

void QCCounters::AppendMultiPass(rdcarray<uint32_t> &needed, rdcarray<uint32_t> &dedicated)
{
  PlanPasses(needed, dedicated, false, true);
}

void QCCounters::PlanPasses(const rdcarray<uint32_t> &needed, const rdcarray<uint32_t> &dedicated,
                            bool singlePass, bool append)
{
  // The layout arithmetic lives in qc_ggpm_plan.h so that it is compiled and exercised by the
  // desktop build as well; this only hands it the resolved bindings and copies the result back.
  std::vector<QCGGPMPlanNeed> planNeeds;
  planNeeds.resize(m_Needs.size());
  for(size_t i = 0; i < m_Needs.size(); i++)
  {
    planNeeds[i].group = m_Needs[i].group;
    planNeeds[i].counter = m_Needs[i].counter;
    planNeeds[i].maxActive = m_Needs[i].maxActive;
    planNeeds[i].type = m_Needs[i].type;
    planNeeds[i].resolved = m_Needs[i].resolved;
  }

  std::vector<uint32_t> planNeeded, planDedicated;
  for(uint32_t n : needed) planNeeded.push_back(n);
  for(uint32_t n : dedicated) planDedicated.push_back(n);

  std::vector<uint32_t> enabledMetrics;
  for(uint32_t enabled : m_EnabledCounters)
  {
    if(size_t(enabled) < m_Built.size())
      enabledMetrics.push_back(m_Built[enabled].metric);
  }

  std::vector<std::vector<uint32_t>> passes;
  QCGGPMPlanPasses(planNeeded, planDedicated, planNeeds.empty() ? NULL : &planNeeds[0],
                   planNeeds.size(), enabledMetrics, m_AdrenoPart, !singlePass, passes);

  if(!append)
    m_Passes.clear();

  for(size_t p = 0; p < passes.size(); p++)
  {
    rdcarray<uint32_t> pass;
    for(size_t i = 0; i < passes[p].size(); i++)
      pass.push_back(passes[p][i]);
    m_Passes.push_back(pass);
  }
}

void QCCounters::BuildPasses()
{
  m_Passes.clear();

  if(m_EnabledCounters.empty())
    return;

  rdcarray<uint32_t> needed;
  rdcarray<uint32_t> dedicated;
  CollectNeeds(needed, dedicated);

  if(needed.empty() && dedicated.empty())
    return;

  // Both layouts come from QCGGPMPlanPasses: with several passes the counters are split by block
  // capacity, and with one pass everything goes in together - which is the default here, because
  // Adreno exposes a single performance monitor and a monitor created after another one in the same
  // context reports no result at all. See qc_ggpm_plan.h for the full reasoning.
  PlanPasses(needed, dedicated, !MultiPassEnabled(), false);
}

uint32_t QCCounters::GetPassCount()
{
  if(m_Passes.empty())
    BuildPasses();

  // A counter refused during an earlier pass of this fetch gets its own pass appended here, at the
  // point where no sample is in flight: existing pass indices must not move, because the caller is
  // iterating over them as it samples. Only useful with several passes: a device that takes all the
  // counters in one pass is a device where a second monitor reports nothing, so appending a pass for
  // a refused counter there would cost a full replay and still measure nothing.
  if(m_PassesDirty && MultiPassEnabled())
  {
    for(uint32_t needIndex : m_PendingPassCounters)
    {
      bool present = false;
      for(const rdcarray<uint32_t> &pass : m_Passes)
        present = present || pass.contains(needIndex);

      if(present)
        continue;

      rdcarray<uint32_t> single;
      single.push_back(needIndex);
      m_Passes.push_back(single);
    }

    m_PendingPassCounters.clear();
    m_PassesDirty = false;
  }
  else if(m_PassesDirty)
  {
    m_PendingPassCounters.clear();
    m_PassesDirty = false;
  }

  return uint32_t(m_Passes.size());
}

void QCCounters::BeginPass(uint32_t passID)
{
  m_OpenPass = passID;
  m_PassMisses = 0;
  m_PassAbandoned = false;
  m_PassGoodSamples = 0;

  MakeCurrent();

  if(passID >= m_Passes.size())
    return;

  // A monitor is created per pass so the selection cannot accumulate across passes: the driver's
  // select call only adds to the current selection.
  if(!BuildMonitorSelection(m_Passes[passID]))
  {
    // Nothing could be selected. Sampling with the half-built monitor would ask the driver for
    // results of a selection it never accepted, so the pass is skipped: BeginSample() bails out
    // while m_Monitor is 0 and the metrics that need these counters report no value.
    if(m_Monitor != 0 && m_Perf.delete_monitors != NULL)
    {
      PFN_glDeletePerfMonitorsAMD del = (PFN_glDeletePerfMonitorsAMD)m_Perf.delete_monitors;
      del(1, &m_Monitor);
    }
    m_Monitor = 0;
    RDCWARN("QCCounters: pass %u has no usable selection, its counters read no value", passID);
    return;
  }

  // Adreno publishes nothing for a monitor that was not started while the global mode enable is
  // on, and the counter data query then crashes inside the driver. The harness that proved the
  // counter set on hardware enables it between the selection and the begin call, so the same order
  // is kept here. The enable is left on for the next pass as well, see m_GlobalModeOn.
  SetGlobalMode(true);

  if(QCDebugLogEnabled())
  {
    PFN_glGetError getError = (PFN_glGetError)m_Perf.get_error;
    RDCLOG("QCCounters: pass %u monitor %u selected %u globalModeError %d", passID, m_Monitor,
           (uint32_t)m_Selected.size(),
           getError != NULL ? int(getError()) : -1);
  }
}

void QCCounters::SetGlobalMode(bool enabled)
{
  if(enabled == m_GlobalModeOn)
    return;

  if(enabled)
  {
    if(m_Perf.gl_enable == NULL)
      return;
    typedef void (*PFN_glEnable)(uint32_t cap);
    ((PFN_glEnable)m_Perf.gl_enable)(GL_PERFMON_GLOBAL_MODE_QCOM);
  }
  else
  {
    if(m_Perf.gl_disable == NULL)
      return;
    typedef void (*PFN_glDisable)(uint32_t cap);
    ((PFN_glDisable)m_Perf.gl_disable)(GL_PERFMON_GLOBAL_MODE_QCOM);
  }

  m_GlobalModeOn = enabled;
}

bool QCCounters::BuildMonitorSelection(const rdcarray<uint32_t> &pass)
{
  PFN_glGenPerfMonitorsAMD gen = (PFN_glGenPerfMonitorsAMD)m_Perf.gen_monitors;
  PFN_glDeletePerfMonitorsAMD del = (PFN_glDeletePerfMonitorsAMD)m_Perf.delete_monitors;
  PFN_glSelectPerfMonitorCountersAMD select =
      (PFN_glSelectPerfMonitorCountersAMD)m_Perf.select_counters;
  PFN_glGetError getError = (PFN_glGetError)m_Perf.get_error;

  if(gen == NULL || select == NULL)
    return false;

  m_Selected.clear();

  // A pass with a single counter is the fallback for a counter the driver refuses to select next to
  // the others, so that one is still selected even though it is marked refused.
  const bool dedicated = pass.size() == 1;

  // One counter at a time. A rejected select also leaves the monitor in an unknown state, so the
  // whole selection is rebuilt from scratch after each refusal, minus the refused counter.
  for(uint32_t attempt = 0; attempt < 8; attempt++)
  {
    if(m_Monitor != 0)
    {
      if(del != NULL)
        del(1, &m_Monitor);
      m_Monitor = 0;
    }

    gen(1, &m_Monitor);

    if(m_Monitor == 0)
    {
      RDCWARN("QCCounters: the driver did not create a perf monitor");
      return false;
    }

    m_Selected.clear();
    bool refused = false;

    for(size_t i = 0; i < pass.size(); i++)
    {
      const uint32_t needIndex = pass[i];

      // A refused counter is still sampled when it is alone in the pass, which is exactly what the
      // dedicated pass is for.
      if(needIndex >= m_Refused.size() || (m_Refused[needIndex] && !dedicated))
        continue;

      const NeedBinding &binding = m_Needs[needIndex];

      // Drain stale errors first, otherwise an error from unrelated GL work would read as a refusal.
      if(getError != NULL)
      {
        for(int drain = 0; drain < 32 && getError() != GL_NO_ERROR; drain++)
        {
        }
      }

      uint32_t counter = binding.counter;
      select(m_Monitor, 1, binding.group, 1, &counter);

      if(getError != NULL && getError() != GL_NO_ERROR)
      {
        MarkRefused(needIndex);
        refused = true;
        break;
      }

      m_Selected.push_back(needIndex);
    }

    if(!refused)
      return true;
  }

  RDCWARN("QCCounters: could not build a perf monitor selection for this pass");
  return false;
}

void QCCounters::MarkRefused(uint32_t needIndex)
{
  if(needIndex >= m_Refused.size() || m_Refused[needIndex])
    return;

  m_Refused[needIndex] = 1;

  if(!m_PendingPassCounters.contains(needIndex))
    m_PendingPassCounters.push_back(needIndex);

  m_PassesDirty = true;

  const char *name = QCGGPMCounterName(kQCGGPMNeeds[needIndex].block, kQCGGPMNeeds[needIndex].id);

  RDCLOG("QCCounters: the driver refuses to select %s[%u] (\"%s\") together with the other counters "
         "of its block, so it is measured in a pass of its own",
         kQCGGPMNeeds[needIndex].block, kQCGGPMNeeds[needIndex].id, name != NULL ? name : "?");
}

void QCCounters::EndPass()
{
  MakeCurrent();

  m_Selected.clear();

  // Everything the pass submitted has to be done with before the monitor goes away, otherwise the
  // next monitor is set up while the previous one still owns the hardware.
  if(m_Perf.gl_finish != NULL)
  {
    typedef void (*PFN_glFinish)();
    ((PFN_glFinish)m_Perf.gl_finish)();
  }

  // The monitor is released and the global mode switched off between passes, the same way the end of
  // a fetch leaves them: a monitor set up while either is still in place from the previous pass
  // reports no result for any window on this device.
  ReleaseMonitors();
  SetGlobalMode(false);
}

void QCCounters::ReleaseMonitors()
{
  if(m_Monitor != 0)
  {
    m_OpenMonitors.push_back(m_Monitor);
    m_Monitor = 0;
  }

  if(!m_OpenMonitors.empty() && m_Perf.delete_monitors != NULL)
  {
    MakeCurrent();
    PFN_glDeletePerfMonitorsAMD del = (PFN_glDeletePerfMonitorsAMD)m_Perf.delete_monitors;
    del((uint32_t)m_OpenMonitors.size(), m_OpenMonitors.data());
    m_OpenMonitors.clear();
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
  if(m_Monitor == 0 || m_Sampling || m_PassAbandoned)
    return;

  MakeCurrent();

  PFN_glBeginPerfMonitorAMD begin = (PFN_glBeginPerfMonitorAMD)m_Perf.begin;
  begin(m_Monitor);

  // Push the start marker to the GPU now. When the work being measured is submitted from another
  // API (a Vulkan replay) nothing else would flush this context before that work runs, and the
  // window would then start after it. A GL replay submits its own work in this context, and an
  // extra flush inside the window makes the Adreno driver lose the monitor's result storage.
  if(m_OwnContext && m_Perf.flush)
  {
    typedef void (*PFN_glFlush)();
    ((PFN_glFlush)m_Perf.flush)();
  }

  m_CurrentEvent = eventId;
  m_SampleStartSec = NowSeconds();
  m_Sampling = true;

  if(QCDebugLogEnabled())
    RDCLOG("QCCounters: begin event %u monitor %u", eventId, m_Monitor);
}

void QCCounters::EndSample()
{
  if(!m_Sampling)
    return;

  m_Sampling = false;

  MakeCurrent();

  PFN_glEndPerfMonitorAMD end = (PFN_glEndPerfMonitorAMD)m_Perf.end;
  end(m_Monitor);

  // Push the end marker to the GPU. An event that replays to no GL call at all -- the last events of
  // a frame usually do -- would otherwise leave the marker in the command buffer, and the monitor
  // would never report a result for that window. The flush is outside the measured window, so it
  // cannot change what the counters cover.
  if(m_Perf.flush != NULL)
  {
    typedef void (*PFN_glFlush)();
    ((PFN_glFlush)m_Perf.flush)();
  }

  if(QCDebugLogEnabled())
  {
    PFN_glGetError getError = (PFN_glGetError)m_Perf.get_error;
    RDCLOG("QCCounters: end event %u monitor %u endError %d", m_CurrentEvent, m_Monitor,
           getError != NULL ? int(getError()) : -1);
  }

  // The caller has already waited on a fence, so the GPU has finished this draw and the result
  // should be ready. The wait below is a bounded safety net for the case where the driver defers
  // publication; when it expires the sample is dropped rather than filled with stale values.
  PFN_glGetPerfMonitorCounterDataAMD getData =
      (PFN_glGetPerfMonitorCounterDataAMD)m_Perf.get_counter_data;

  // The window has to be measured before the driver is queried: the query itself can wait, and the
  // window the counters cover is the one from begin to end.
  const double window = NowSeconds() - m_SampleStartSec;

  uint32_t available = 0;
  const double waitEnd = NowSeconds() + 2.0;
  while(NowSeconds() < waitEnd)
  {
    getData(m_Monitor, GL_PERFMON_RESULT_AVAILABLE_AMD, sizeof(available), &available, NULL);
    if(available)
      break;
    usleep(1000);
  }

  if(QCDebugLogEnabled())
    RDCLOG("QCCounters: event %u available %u", m_CurrentEvent, available);

  if(!available)
  {
    m_PassMisses++;

    RDCWARN("QCCounters: no result for event %u, counters for this draw are unavailable",
            m_CurrentEvent);

    // A single pass that never reports anything may simply be a selection this device will not take
    // as a whole. The multi-pass layout is appended then, so the same fetch still measures what it
    // can instead of losing every counter to one oversized selection.
    if(m_OpenPass == 0 && m_PassGoodSamples == 0 && !MultiPassEnabled() && !m_FellBackToMultiPass)
    {
      m_FellBackToMultiPass = true;

      rdcarray<uint32_t> needed;
      rdcarray<uint32_t> dedicated;
      CollectNeeds(needed, dedicated);
      AppendMultiPass(needed, dedicated);

      RDCWARN("QCCounters: the driver took no counter of the single pass, so the %u counters are "
              "spread over %u passes instead",
              (uint32_t)needed.size(), (uint32_t)m_Passes.size());
    }

    // One event with no result can be the driver deferring publication. Two in a row mean the driver
    // dropped the whole selection when the monitor was set up: every remaining event of this pass
    // would wait out the same timeout to get nothing, so the rest of the pass is skipped and the
    // counters it holds report no value.
    if(m_PassMisses >= 2 && !m_PassAbandoned)
    {
      m_PassAbandoned = true;

      // A pass that measured nothing at all lost its selection; one that measured most of the frame
      // and then ran dry simply reached the events at the end of a frame, which replay to no GL call
      // and therefore produce no window for the monitor to report. Only the first case is a warning.
      if(m_PassGoodSamples == 0)
      {
        const rdcarray<uint32_t> &pass = m_Passes[m_OpenPass < m_Passes.size() ? m_OpenPass : 0];
        rdcarray<char> names;
        for(uint32_t needIndex : pass)
        {
          const char *name = QCGGPMCounterName(kQCGGPMNeeds[needIndex].block,
                                               kQCGGPMNeeds[needIndex].id);
          if(name != NULL)
          {
            if(!names.empty())
              names.append(", ", 2);
            names.append(name, strlen(name));
          }
        }
        names.push_back(0);

        RDCWARN("QCCounters: the driver took none of the %u counters of pass %u in two attempts, so "
                "its remaining events are skipped; these counters read no value in this fetch: %s",
                (uint32_t)pass.size(), m_OpenPass, names.data());
      }
      else
      {
        RDCLOG("QCCounters: pass %u measured %u events and then reported no result for %u in a row, "
               "which is how the frame's trailing events read; the pass stops here",
               m_OpenPass, m_PassGoodSamples, m_PassMisses);
      }
    }

    return;
  }

  m_PassMisses = 0;
  m_PassGoodSamples++;

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

  // The window of the first pass that measured this event is kept, not the last one: a dedicated
  // pass holds a single counter and therefore finishes sooner, and a per-second metric would read
  // high if it were divided by that shorter window instead of by the pass that measured most of its
  // counters.
  if(sample.dt <= 0.0)
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
  if(cached.dt <= 0.0)
    cached.dt = window;
  for(size_t i = 0; i < sample.raw.size() && i < cached.raw.size(); i++)
  {
    if(sample.raw[i] >= 0.0)
      cached.raw[i] = sample.raw[i];
  }

  // Only a counter that really reported a value counts as measured. A counter of the pass that came
  // back empty has to be tried again by the next fetch: marking it attempted here would answer every
  // later fetch for this event from the cache with no value at all, which is how a fetch could end up
  // returning nothing for counters an earlier fetch measured.
  const rdcarray<uint32_t> &openPass = m_Passes[m_OpenPass < m_Passes.size() ? m_OpenPass : 0];
  for(size_t i = 0; i < openPass.size(); i++)
  {
    const uint32_t need = openPass[i];
    if(need < uint32_t(cached.attempted.size()) && need < cached.raw.size() && cached.raw[need] >= 0.0)
      cached.attempted[need] = 1;
  }
}

bool QCCounters::ReadResults(const rdcarray<uint32_t> &needIndices, const uint32_t *words,
                             size_t wordCount, double *out)
{
  // The record grammar and its validation live in qc_ggpm_plan.h, next to the plan that produced the
  // selection, so both halves of the protocol are covered by the same desktop-side self check.
  std::vector<QCGGPMPlanNeed> planNeeds;
  planNeeds.resize(m_Needs.size());
  for(size_t i = 0; i < m_Needs.size(); i++)
  {
    planNeeds[i].group = m_Needs[i].group;
    planNeeds[i].counter = m_Needs[i].counter;
    planNeeds[i].maxActive = m_Needs[i].maxActive;
    planNeeds[i].type = m_Needs[i].type;
    planNeeds[i].resolved = m_Needs[i].resolved;
  }

  std::vector<uint32_t> indices;
  for(uint32_t n : needIndices) indices.push_back(n);

  return QCGGPMDecodeResults(words, wordCount, planNeeds.empty() ? NULL : &planNeeds[0],
                             planNeeds.size(), indices.empty() ? NULL : &indices[0], indices.size(),
                             out);
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

  ReleaseMonitors();

  // The counters are all read, so the driver can stop recording. Leaving the enable on would keep
  // every later monitor in this context able to publish results, which the glGetPerfMonitorCounterData
  // call assumes it may read, but it also keeps the driver's result buffer allocated for no reason.
  SetGlobalMode(false);

  return ret;
}

#endif    // ENABLED(RDOC_ANDROID)

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

// The Adreno GGPM derived-metric engine.
//
// Snapdragon Profiler does not read "% Shaders Busy" out of the hardware. The hardware
// exposes raw block counters and the profiler computes the metric on top of them, in the
// per-metric `getComputedValue()` bodies of libGGPM.so. This header is the RenderDoc-side
// re-implementation of that layer: the metric list, the raw counters each metric reads,
// the per-device constants the formulas divide by, and the arithmetic itself.
//
// Provenance of every piece here:
//   * the metric list, the raw counter ids and the arithmetic come from the AArch64
//     decompilation of libGGPM.so (hwl::Metric::<Name>::getComputedValue and the
//     Percent*/Ratio* helpers), not from the metric names;
//   * the counter name tables in qc_ggpm_names.h come from the plugin's own name arrays;
//   * the per-device constants (numShaderPipes / numTexturePipes / numAluPerShaderPipe /
//     numEfuBlocksPerShaderPipe / device id table) are the decompiled switch bodies of
//     those functions in the same library.
//
// Two conventions are inherited from libGGPM and must be preserved:
//   * an undeclared/unavailable counter reads as -1.0 (hwl::Metric::countableValue
//     returns exactly that sentinel), and a formula that touches it produces no value at
//     all rather than a zero;
//   * hwl::Metric::percent() multiplies by 100 and then clamps only the 100 < v < 103
//     band to 100. Values above that are reported unchanged even when they look odd
//     (e.g. "% Texture L2 Miss" legitimately reports above 100% because it is a byte
//     ratio, not a hit rate).
//
// This header is self contained (no RenderDoc or platform headers) so the engine can be
// compiled and cross-checked against the device outside the renderer.

#pragma once

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "qc_ggpm_names.h"

// ---------------------------------------------------------------------------------------------
// Raw counters: one entry per (block, countable id) pair that some metric expression reads.
// This list, not the metric table, decides what gets selected on the device - several
// metrics read a block other than the one they are named after.
// ---------------------------------------------------------------------------------------------

struct QCGGPMNeed
{
  const char *block;
  uint32_t id;
};

static const QCGGPMNeed kQCGGPMNeeds[] = {
    {"RBBM", 6},  {"CP", 0},   {"CP", 1},   {"CP", 3},    {"CP", 4},    {"CP", 5},
    {"CP", 6},    {"PC", 2},   {"PC", 26},  {"TSE", 6},   {"TSE", 8},   {"TSE", 9},
    {"UCHE", 1},  {"UCHE", 2}, {"UCHE", 3}, {"UCHE", 4},  {"UCHE", 5},  {"UCHE", 8},
    {"UCHE", 9},  {"UCHE", 13}, {"UCHE", 15}, {"UCHE", 18}, {"UCHE", 34}, {"UCHE", 35},
    {"TP", 0},    {"TP", 6},   {"TP", 7},   {"TP", 10},   {"TP", 25},   {"TP", 26},
    {"TP", 28},   {"TP", 29},  {"SP", 0},   {"SP", 1},    {"SP", 2},    {"SP", 4},
    {"SP", 7},    {"SP", 8},   {"SP", 9},   {"SP", 27},   {"SP", 28},   {"SP", 29},
    {"SP", 30},   {"SP", 31},  {"SP", 32},  {"SP", 35},   {"SP", 40},   {"SP", 41},
    {"SP", 51},   {"SP", 52},  {"SP", 63},  {"SP", 74},   {"SP", 76},   {"SP", 78},
    {"CMP", 7},   {"ALWAYSON", 0},
};

static const uint32_t kQCGGPMNeedCount = sizeof(kQCGGPMNeeds) / sizeof(kQCGGPMNeeds[0]);

// ---------------------------------------------------------------------------------------------
// Metric declarations. The arithmetic for each key lives in QCGGPMEvaluate() so a reader can
// compare it line by line with the corresponding decompiled getComputedValue() body.
// ---------------------------------------------------------------------------------------------

struct QCGGPMMetric
{
  const char *key;
  const char *label;
  const char *unit;
  const char *category;
  const char *description;
};

// The categories below are RenderDoc's own grouping of Snapdragon Profiler's metrics (the
// profiler's 44 MetricCategory rows are not reproduced here); they exist so the counter
// picker can group the list, and carry no claim of matching the profiler's UI verbatim.
static const char *kQCGGPMCatGeneral = "GPU General";
static const char *kQCGGPMCatShader = "GPU Shader Processing";
static const char *kQCGGPMCatPrimitive = "GPU Primitive Processing";
static const char *kQCGGPMCatMemory = "GPU Memory";
static const char *kQCGGPMCatStalls = "GPU Stalls";

static const QCGGPMMetric kQCGGPMMetrics[] = {
    {"gpu_utilization", "GPU % Utilization", "%", kQCGGPMCatGeneral,
     "Time the GPU was busy, as a fraction of wall time scaled by the maximum GPU clock. "
     "Uses RBBM timebase cycles over (window * max clock), the same expression as Snapdragon "
     "Profiler, so a GPU idling far below its maximum clock reports a low value."},
    {"gpu_busy", "% Busy", "%", kQCGGPMCatGeneral,
     "Graphics core busy cycles (RBBM status masked) over the command processor always-on "
     "cycle count."},
    {"cp_overhead", "% CP Overhead", "%", kQCGGPMCatGeneral,
     "Command processor busy cycles while the graphics core is idle, over the always-on cycle "
     "count."},
    {"vertex_fetch_stall", "% Vertex Fetch Stall", "%", kQCGGPMCatStalls,
     "Primitive/parameter cache data-call busy cycles over the RBBM timebase."},
    {"texture_fetch_stall", "% Texture Fetch Stall", "%", kQCGGPMCatStalls,
     "Shader texture fetches per texture pipe over the RBBM timebase."},
    {"icache_miss", "% Instruction Cache Miss", "%", kQCGGPMCatShader,
     "Shader instruction cache misses over instruction cache requests."},
    {"shaders_busy", "% Shaders Busy", "%", kQCGGPMCatShader,
     "The busier of the shader pipe and texture pipe utilisation, over the RBBM timebase."},
    {"shaders_stalled", "% Shaders Stalled", "%", kQCGGPMCatStalls,
     "Shader cycles spent with no wave issued, over the RBBM timebase."},
    {"texture_pipes_busy", "% Texture Pipes Busy", "%", kQCGGPMCatShader,
     "Texture pipe busy cycles over the RBBM timebase."},
    {"shader_alu_capacity", "% Shader ALU Capacity Utilized", "%", kQCGGPMCatShader,
     "Full precision ALU slots issued (half precision counted as half a slot) over the ALU "
     "capacity of every shader pipe."},
    {"kernel_alu_capacity", "% Kernel ALU Capacity Utilized", "%", kQCGGPMCatShader,
     "The same capacity ratio restricted to compute kernels."},
    {"load_store_utilization", "Load-Store Utilization", "%", kQCGGPMCatShader,
     "Load-store unit working cycles over shader pipe busy cycles, scaled by a per device "
     "factor."},
    {"time_alus_working", "% Time ALUs Working", "%", kQCGGPMCatShader,
     "Cycles any ALU in a shader pipe was working, over shader pipe busy cycles."},
    {"time_efus_working", "% Time EFUs Working", "%", kQCGGPMCatShader,
     "EFU working cycles per EFU block, over shader pipe busy cycles."},
    {"wave_context_occupancy", "% Wave Context Occupancy", "%", kQCGGPMCatShader,
     "Occupied wave slots over total wave slots (wave contexts of eight)."},
    {"time_shading_fragments", "% Time Shading Fragments", "%", kQCGGPMCatShader,
     "Fragment shader stage cycles less the compute share, over the vertex plus fragment "
     "stage cycle count."},
    {"time_shading_vertices", "% Time Shading Vertices", "%", kQCGGPMCatShader,
     "Vertex shader stage cycles over the vertex plus fragment stage cycle count."},
    {"time_compute", "% Time Compute", "%", kQCGGPMCatShader,
     "Compute stage cycles over the vertex plus fragment stage cycle count."},
    {"l1_hit", "% L1 Hit", "%", kQCGGPMCatMemory,
     "Texture requests that hit L1 (requests minus L1 misses) over requests."},
    {"texture_l1_miss", "% Texture L1 Miss", "%", kQCGGPMCatMemory,
     "Texture L1 misses over texture requests."},
    {"texture_l2_miss", "% Texture L2 Miss", "%", kQCGGPMCatMemory,
     "L2 fill bytes over requested bytes. This is a byte ratio rather than a hit rate, so it "
     "can legitimately exceed 100% - the same value Snapdragon Profiler reports."},
    {"nearest_filtered", "% Nearest Filtered", "%", kQCGGPMCatMemory,
     "Texels fetched with nearest filtering, over all texels fetched."},
    {"linear_filtered", "% Linear Filtered", "%", kQCGGPMCatMemory,
     "Texels fetched with linear filtering, over all texels fetched."},
    {"anisotropic_filtered", "% Anisotropic Filtered", "%", kQCGGPMCatMemory,
     "Texels fetched with anisotropic filtering, over all texels fetched."},
    {"non_base_level", "% Non-Base Level Textures", "%", kQCGGPMCatMemory,
     "Texels fetched from a mip level other than the base level, over all texels fetched."},
    {"prims_trivially_rejected", "% Prims Trivially Rejected", "%", kQCGGPMCatPrimitive,
     "Primitives rejected by the rasteriser before setup, over primitives submitted."},
    {"prims_clipped", "% Prims Clipped", "%", kQCGGPMCatPrimitive,
     "Primitives clipped, over primitives submitted."},
    {"global_buffer_read_l2_hit", "% Global Buffer Read L2 Hit", "%", kQCGGPMCatMemory,
     "Global buffer read requests against L2 read beats, two beats per request."},
    {"global_buffer_write_l2_hit", "% Global Buffer Write L2 Hit", "%", kQCGGPMCatMemory,
     "Global buffer write evictions against write requests."},
    {"image_read_l2_hit", "% Image Read L2 Hit", "%", kQCGGPMCatMemory,
     "Image read beats against L2 read requests."},
    {"avg_memory_latency", "Avg Memory Latency Cycles", "cyc", kQCGGPMCatMemory,
     "Mean memory latency in cycles over the UCHE and compression latency sample counts."},
    {"avg_bytes_per_vertex", "Avg Bytes / Vertex", "B", kQCGGPMCatPrimitive,
     "Vertex fetch bytes over vertices shaded."},
    {"avg_load_store_per_cycle", "Avg Load-Store Instructions / Cycle", "inst/cyc",
     kQCGGPMCatShader,
     "Local and global load/store instructions per shader pipe cycle."},
    {"texture_read_bw", "Texture Memory Read BW", "B/s", kQCGGPMCatMemory,
     "Texture and compressed image read bytes over the sample window."},
    {"vertex_read_bw", "Vertex Memory Read", "B/s", kQCGGPMCatMemory,
     "Vertex fetch read bytes over the sample window."},
    {"sp_read_bw", "SP Memory Read", "B/s", kQCGGPMCatMemory,
     "Shader processor read bytes over the sample window."},
    {"global_buffer_read_bw", "Global Buffer Read BW", "B/s", kQCGGPMCatMemory,
     "Global buffer read bytes over the sample window."},
    {"global_buffer_read_req_bw", "Global Buffer Read Request BW", "B/s", kQCGGPMCatMemory,
     "Global buffer read request bytes over the sample window."},
    {"global_image_read_bw", "Global Image Uncompressed Read BW", "B/s", kQCGGPMCatMemory,
     "Uncompressed image read bytes over the sample window."},
    {"global_image_read_req_bw", "Global Image Read Request BW", "B/s", kQCGGPMCatMemory,
     "Image read request bytes over the sample window."},
    {"gpu_bytes_write_requested", "Bytes Data Write Requested", "B", kQCGGPMCatMemory,
     "Bytes requested by writes to L2."},
    {"gpu_bytes_written", "Bytes Data Actually Written", "B", kQCGGPMCatMemory,
     "Bytes actually evicted to memory after L2 write coalescing."},
    {"preemptions_per_second", "Preemptions / Second", "/s", kQCGGPMCatGeneral,
     "Command processor preemptions per second over the sample window."},
    {"avg_preemption_delay", "Avg Preemption Delay", "us", kQCGGPMCatGeneral,
     "Mean preemption reaction delay, converted to microseconds with the always-on "
     "cycle rate."},
    {"avg_preemption_save", "Avg Preemption Save Time", "us", kQCGGPMCatGeneral,
     "Mean preemption context save time, converted to microseconds."},
    {"avg_preemption_restore", "Avg Preemption Restore Time", "us", kQCGGPMCatGeneral,
     "Mean preemption context restore time, converted to microseconds."},
    {"local_memory_load_instructions", "Local Memory Load Instructions", "inst",
     kQCGGPMCatShader, "Wave level local memory load instructions."},
    {"local_memory_store_instructions", "Local Memory Store Instructions", "inst",
     kQCGGPMCatShader, "Wave level local memory store instructions."},
    {"local_memory_atomic_instructions", "Local Memory Atomic Instructions", "inst",
     kQCGGPMCatShader, "Wave level local memory atomic instructions."},
    {"global_memory_load_instructions", "Global Memory Load Instructions", "inst",
     kQCGGPMCatShader, "Wave level global memory load instructions."},
    {"global_memory_store_instructions", "Global Memory Store Instructions", "inst",
     kQCGGPMCatShader, "Wave level global memory store instructions."},
    {"global_memory_atomic_instructions", "Global Memory Atomic Instructions", "inst",
     kQCGGPMCatShader, "Wave level global memory atomic instructions."},
    {"compressed_image_read_bw", "Global Image Compressed Read BW", "B/s", kQCGGPMCatMemory,
     "Compressed image read bytes over the sample window."},
    {"compute_kernel_load_cycles", "% Compute Kernel Load Cycles", "%", kQCGGPMCatShader,
     "RBBM timebase minus shader pipe busy, over the command processor always-on count."},
    {"stalled_on_system_memory", "% Stalled On System Memory", "%", kQCGGPMCatStalls,
     "UCHE stall cycles per texture pipe, over the RBBM timebase."},
};

static const uint32_t kQCGGPMMetricCount = sizeof(kQCGGPMMetrics) / sizeof(kQCGGPMMetrics[0]);

// ---------------------------------------------------------------------------------------------
// Per device constants, transcribed from the decompiled bodies in libGGPM.so.
//
// core::DeviceID is Snapdragon Profiler's own device number, obtained by matching the
// GL_RENDERER string against the plugin's device list ("Adreno (TM) 610" -> A610 -> 0x23).
// The formulas below switch on it to pick how many shader pipes, texture pipes, ALUs per
// pipe and EFU blocks per pipe a part has. An unknown device leaves those at 0, and every
// formula that depends on them then reports no value instead of a wrong one.
// ---------------------------------------------------------------------------------------------

struct QCGGPMDeviceEntry
{
  const char *name;
  uint32_t id;
};

// Transcribed from core::DeviceID::deviceList in libGGPM.so. Only the Adreno 6xx/7xx parts
// whose ids were read out of that table are listed; a part that is missing here simply
// resolves to "unknown device".
static const QCGGPMDeviceEntry kQCGGPMDevices[] = {
    {"A605", 0x21}, {"A608", 0x22}, {"A610", 0x23}, {"A611", 0x24}, {"A612", 0x25},
    {"A613", 0x26}, {"A615", 0x27}, {"A616", 0x28}, {"A618", 0x29}, {"A619", 0x2a},
    {"A620", 0x2b}, {"A621", 0x2c}, {"A622", 0x2d}, {"A623", 0x2e}, {"A630", 0x2f},
    {"A635", 0x30}, {"A640", 0x31}, {"A642", 0x32}, {"A643", 0x34}, {"A644", 0x35},
    {"A645", 0x36}, {"A650", 0x38}, {"A660", 0x39}, {"A680", 0x3c}, {"A690", 0x3d},
};

// Resolve a GL_RENDERER string ("Adreno (TM) 610") to a Snapdragon Profiler device id, or
// 0 when the part is not in the table.
static inline uint32_t QCGGPMDeviceIdFromRenderer(const char *renderer)
{
  if(renderer == NULL)
    return 0;

  const char *p = strstr(renderer, "Adreno (TM) ");
  if(p == NULL)
    return 0;

  p += strlen("Adreno (TM) ");

  char model[16] = {};
  size_t n = 0;
  while(p[n] >= '0' && p[n] <= '9' && n + 1 < sizeof(model))
  {
    model[n] = p[n];
    n++;
  }

  if(n == 0)
    return 0;

  char key[24];
  snprintf(key, sizeof(key), "A%s", model);

  for(size_t i = 0; i < sizeof(kQCGGPMDevices) / sizeof(kQCGGPMDevices[0]); i++)
  {
    if(strcmp(kQCGGPMDevices[i].name, key) == 0)
      return kQCGGPMDevices[i].id;
  }

  return 0;
}

// numShaderPipes / numTexturePipes / numAluPerShaderPipe / numEfuBlocksPerShaderPipe, as
// decompiled from libGGPM.so. The switch bodies are kept in their original shape so they can
// be diffed against the library; do not "simplify" them into a table without re-deriving.
static inline double QCGGPMNumShaderPipes(uint32_t id)
{
  if(id == 0x32)
    return 6.0;

  if(id < 0x33)
  {
    if(id == 0x30)
      return 4.0;

    if(id != 0x31)
    {
      if(id > 0x2a)
        return 2.0;

      return id > 0x20 ? 1.0 : 0.0;
    }

    return 2.0;
  }

  if(id < 0x3c)
    return id > 0x34 ? 6.0 : 4.0;

  if(id == 0x3c)
    return 4.0;

  if(id == 0x3d)
    return 16.0;

  return 0.0;
}

static inline double QCGGPMNumTexturePipes(uint32_t id)
{
  if(id == 0x32)
    return QCGGPMNumShaderPipes(id);

  if(id < 0x33)
  {
    if(id < 0x2f)
    {
      if(id > 0x2a)
        return QCGGPMNumShaderPipes(id);

      if(id < 0x27)
        return id < 0x21 ? 0.0 : QCGGPMNumShaderPipes(id);

      // 0x27..0x2a: twice the shader pipe count.
      return QCGGPMNumShaderPipes(id) * 2.0;
    }

    if(id == 0x30)
      return QCGGPMNumShaderPipes(id);

    if(id == 0x31)
      return QCGGPMNumShaderPipes(id) * 3.0;

    if(id == 0x2f)
      return QCGGPMNumShaderPipes(id) * 2.0;

    return 0.0;
  }

  if(id > 0x3b)
  {
    if(id == 0x3c)
      return QCGGPMNumShaderPipes(id) * 3.0;

    if(id == 0x3d)
      return QCGGPMNumShaderPipes(id);

    return 0.0;
  }

  if(id > 0x34)
    return QCGGPMNumShaderPipes(id);

  return QCGGPMNumShaderPipes(id) * 2.0;
}

static inline double QCGGPMNumAluPerShaderPipe(uint32_t id)
{
  if(id == 0x31 || id == 0x3c)
    return 128.0;

  if(id < 0x32)
  {
    if(id == 0x2f)
      return 256.0;

    if(id == 0x30)
      return 128.0;

    if(id < 0x27)
      return id > 0x20 ? 64.0 : 0.0;

    return 128.0;
  }

  if(id < 0x3d)
    return 128.0;

  return id == 0x3d ? 128.0 : 0.0;
}

static inline double QCGGPMNumEfuBlocksPerShaderPipe(uint32_t id)
{
  if(id > 0x3d)
    return 1.0;

  const unsigned long long mask = 0x2ffd787e00000000ULL;
  const bool listed = id < 64 && ((1ULL << id) & mask) != 0;

  if(!listed)
  {
    if(id == 0x31)
      return 3.0;

    if(id > 0x31)
      return id == 0x3c ? 3.0 : 2.0;

    if(id != 0x21)
    {
      if(id - 0x23 > 1)
        return 2.0;

      return 1.0;
    }
  }

  return 1.0;
}

// ---------------------------------------------------------------------------------------------
// One sample: the raw counter values read over one window, plus the window's wall duration and
// the device constants the formulas need.
//
// values[] is parallel to kQCGGPMNeeds. An entry still at -1.0 means "not measured in this
// window" - the same sentinel libGGPM uses for an undeclared countable - and any formula that
// reads it yields no value.
// ---------------------------------------------------------------------------------------------

struct QCGGPMSample
{
  double values[kQCGGPMNeedCount];
  double dt;
  double maxClockHz;
  uint32_t deviceId;
  bool countersReady;

  void Init()
  {
    for(uint32_t i = 0; i < kQCGGPMNeedCount; i++)
      values[i] = -1.0;

    dt = 0.0;
    maxClockHz = 0.0;
    deviceId = 0;
    countersReady = false;
  }

  // Index of a (block, id) pair in the needs table, or -1 when no metric reads it.
  static int IndexOf(const char *block, uint32_t id)
  {
    for(uint32_t i = 0; i < kQCGGPMNeedCount; i++)
    {
      if(kQCGGPMNeeds[i].id == id && strcmp(kQCGGPMNeeds[i].block, block) == 0)
        return (int)i;
    }

    return -1;
  }

  void Set(const char *block, uint32_t id, double value)
  {
    const int idx = IndexOf(block, id);
    if(idx >= 0)
      values[idx] = value;
  }

  // The libGGPM readable equivalent: an unmet counter is -1.0, never 0.
  double Get(const char *block, uint32_t id) const
  {
    const int idx = IndexOf(block, id);
    return idx >= 0 ? values[idx] : -1.0;
  }
};

// hwl::Metric::percent(): scale to 0..100 and absorb floating point slop just above 100.
// Anything above that band is passed through, matching the library.
static inline double QCGGPMPercent(double v)
{
  double p = v * 100.0;
  if(p > 100.0 && p < 103.0)
    p = 100.0;
  return p;
}

// RDC's guard for "this ratio cannot be measured": a non-positive or non-finite denominator,
// or a negative numerator, means the window is missing a counter rather than being zero.
static inline bool QCGGPMDiv(double num, double den, double *out)
{
  if(!(den > 0.0) || !isfinite(den) || !isfinite(num) || num < 0.0)
    return false;

  *out = num / den;
  return true;
}

// ---------------------------------------------------------------------------------------------
// Metric evaluation. Returns false when the window cannot produce the metric (a counter is
// missing, or the formula's denominator is not positive); callers must not turn that into 0.
// ---------------------------------------------------------------------------------------------

static inline bool QCGGPMEvaluate(const char *key, const QCGGPMSample &s, double *out)
{
  const double dt = s.dt;
  const double maxHz = s.maxClockHz;
  const uint32_t dev = s.deviceId;
  const double sp = s.Get("SP", 0);
  const double tp = s.Get("TP", 0);
  const double timebase = s.Get("RBBM", 6);

  double a, b, c;

  if(strcmp(key, "gpu_utilization") == 0)
  {
    if(!(maxHz > 0.0) || !(dt > 0.0) || timebase < 0.0)
      return false;

    *out = QCGGPMPercent(timebase / (dt * maxHz));
    return true;
  }

  if(strcmp(key, "gpu_busy") == 0)
  {
    if(!QCGGPMDiv(timebase, s.Get("CP", 0), &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "cp_overhead") == 0)
  {
    // CP[1] busy-for-graphics-core-idle cycles over CP[0] always-on cycles.
    if(!QCGGPMDiv(s.Get("CP", 1), s.Get("CP", 0), &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "vertex_fetch_stall") == 0)
  {
    if(!QCGGPMDiv(s.Get("PC", 2), timebase, &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "texture_fetch_stall") == 0)
  {
    const double pipes = QCGGPMNumTexturePipes(dev);
    if(pipes <= 0.0 || !QCGGPMDiv(s.Get("SP", 4) / pipes, timebase, &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "icache_miss") == 0)
  {
    if(!QCGGPMDiv(s.Get("SP", 52), s.Get("SP", 51), &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "shaders_busy") == 0)
  {
    const double pipes = QCGGPMNumShaderPipes(dev);
    const double texpipes = QCGGPMNumTexturePipes(dev);
    if(pipes <= 0.0 || texpipes <= 0.0 || sp < 0.0 || tp < 0.0)
      return false;

    a = sp / pipes;
    b = tp / texpipes;

    // The busier of the shader and texture pipes.
    if(a <= b)
      a = b;

    if(!QCGGPMDiv(a, timebase, &c))
      return false;

    *out = QCGGPMPercent(c);
    return true;
  }

  if(strcmp(key, "shaders_stalled") == 0)
  {
    const double pipes = QCGGPMNumShaderPipes(dev);
    if(pipes <= 0.0 || !QCGGPMDiv(s.Get("SP", 7) / pipes, timebase, &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "texture_pipes_busy") == 0)
  {
    const double pipes = QCGGPMNumTexturePipes(dev);
    if(pipes <= 0.0 || !QCGGPMDiv(tp / pipes, timebase, &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "shader_alu_capacity") == 0 || strcmp(key, "kernel_alu_capacity") == 0)
  {
    const bool vs = strcmp(key, "shader_alu_capacity") == 0;
    const double pipes = QCGGPMNumShaderPipes(dev);
    const double alu = QCGGPMNumAluPerShaderPipe(dev);
    const double half = s.Get("SP", 41);
    const double fsFull = s.Get("SP", 40);
    const double vsFull = vs ? s.Get("SP", 35) : 0.0;

    if(pipes <= 0.0 || alu <= 0.0 || sp <= 0.0 || half < 0.0 || fsFull < 0.0 ||
       (vs && vsFull < 0.0))
      return false;

    // Work in full precision ALU slots: one half precision op is half a slot.
    a = (half * 4.0) * 0.5 + (fsFull + vsFull) * 4.0;

    if(!QCGGPMDiv(a, (alu * pipes * sp) / pipes, &b))
      return false;

    *out = QCGGPMPercent(b);
    return true;
  }

  if(strcmp(key, "load_store_utilization") == 0 || strcmp(key, "time_alus_working") == 0)
  {
    const bool ls = strcmp(key, "load_store_utilization") == 0;
    const double working = ls ? s.Get("SP", 63) : s.Get("SP", 1);

    if(!QCGGPMDiv(working, sp, &a))
      return false;

    // Both metrics scale by a per device factor: 1.0 for 0x21 and 0x23..0x24, 0.5 by default,
    // 1/3 for 0x31 and 0x3c (LoadStoreUtilization::getComputedValue).
    double factor = 0.5;
    if(dev == 0x31 || dev == 0x3c)
      factor = 1.0 / 3.0;
    else if(dev == 0x21 || dev - 0x23 < 2)
      factor = 1.0;

    *out = QCGGPMPercent(a * factor);
    return true;
  }

  if(strcmp(key, "time_efus_working") == 0)
  {
    const double blocks = QCGGPMNumEfuBlocksPerShaderPipe(dev);
    if(blocks <= 0.0 || !QCGGPMDiv(s.Get("SP", 2) / blocks, sp, &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "wave_context_occupancy") == 0)
  {
    if(!QCGGPMDiv(s.Get("SP", 8), s.Get("SP", 9), &a))
      return false;

    *out = QCGGPMPercent(a * 0.0625);
    return true;
  }

  if(strcmp(key, "time_shading_fragments") == 0 || strcmp(key, "time_shading_vertices") == 0 ||
     strcmp(key, "time_compute") == 0)
  {
    const double fsStage = s.Get("SP", 74);
    const double vsStage = s.Get("SP", 76);
    const double csStage = s.Get("SP", 78);

    if(fsStage < 0.0 || vsStage < 0.0 || csStage < 0.0)
      return false;

    if(!QCGGPMDiv(vsStage + fsStage, 1.0, &a) || !(a > 0.0))
      return false;

    if(strcmp(key, "time_shading_fragments") == 0)
      b = fsStage - csStage;
    else if(strcmp(key, "time_shading_vertices") == 0)
      b = vsStage;
    else
      b = csStage;

    *out = QCGGPMPercent(b / a);
    return true;
  }

  if(strcmp(key, "l1_hit") == 0)
  {
    const double req = s.Get("TP", 6);
    const double miss = s.Get("TP", 7);

    if(req < 0.0 || miss < 0.0 || !QCGGPMDiv(req - miss, req, &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "texture_l1_miss") == 0)
  {
    if(!QCGGPMDiv(s.Get("TP", 7), s.Get("TP", 6), &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "texture_l2_miss") == 0)
  {
    const double beats = s.Get("UCHE", 4);
    const double req = s.Get("UCHE", 9);

    if(beats < 0.0 || req <= 0.0)
      return false;

    *out = QCGGPMPercent((beats * 32.0) / (req * 16.0));
    return true;
  }

  if(strcmp(key, "nearest_filtered") == 0 || strcmp(key, "linear_filtered") == 0 ||
     strcmp(key, "anisotropic_filtered") == 0)
  {
    const uint32_t id = strcmp(key, "nearest_filtered") == 0
                            ? 25
                            : (strcmp(key, "linear_filtered") == 0 ? 26 : 28);

    if(!QCGGPMDiv(s.Get("TP", id), s.Get("TP", 10), &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "non_base_level") == 0)
  {
    const double total = s.Get("TP", 10);
    const double base = s.Get("TP", 29);

    if(total < 0.0 || base < 0.0 || !QCGGPMDiv(total - base, total, &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "prims_trivially_rejected") == 0)
  {
    if(!QCGGPMDiv(s.Get("TSE", 8), s.Get("TSE", 6), &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "prims_clipped") == 0)
  {
    if(!QCGGPMDiv(s.Get("TSE", 9), s.Get("TSE", 6), &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  if(strcmp(key, "global_buffer_read_l2_hit") == 0)
  {
    // UCHE[13] read requests against UCHE[8] beats, two beats per request.
    const double req = s.Get("UCHE", 13);
    const double beats = s.Get("UCHE", 8);

    if(req < 0.0 || beats < 0.0)
      return false;

    if(req < beats + beats)
      *out = QCGGPMPercent(1.0 - req / (beats + beats));
    else
      *out = QCGGPMPercent(0.0);

    return true;
  }

  if(strcmp(key, "global_buffer_write_l2_hit") == 0)
  {
    const double evicts = s.Get("UCHE", 18);
    const double req = s.Get("UCHE", 15);

    if(evicts < 0.0 || req < 0.0)
      return false;

    if(evicts < req)
      *out = QCGGPMPercent(1.0 - evicts / req);
    else
      *out = QCGGPMPercent(0.0);

    return true;
  }

  if(strcmp(key, "image_read_l2_hit") == 0)
  {
    const double beats = s.Get("UCHE", 4);
    const double req = s.Get("UCHE", 9);

    if(beats < 0.0 || req < 0.0)
      return false;

    if(beats < req + req)
      *out = QCGGPMPercent(1.0 - beats / (req + req));
    else
      *out = QCGGPMPercent(0.0);

    return true;
  }

  if(strcmp(key, "avg_memory_latency") == 0)
  {
    const double vbifCyc = s.Get("UCHE", 2);
    const double vbifSamples = s.Get("UCHE", 3);
    const double dcmpCyc = s.Get("UCHE", 35);
    const double dcmpSamples = s.Get("UCHE", 34);

    if(vbifCyc < 0.0 || vbifSamples < 0.0 || dcmpCyc < 0.0 || dcmpSamples < 0.0)
      return false;

    double samples = 0.0;
    if(!QCGGPMDiv(vbifSamples + dcmpSamples, 1.0, &samples) || !(samples > 0.0))
      return false;

    *out = (dcmpCyc + vbifCyc) / samples;
    return true;
  }

  if(strcmp(key, "avg_bytes_per_vertex") == 0)
  {
    if(!QCGGPMDiv(s.Get("UCHE", 5) * 32.0, s.Get("PC", 26), &a))
      return false;

    *out = a;
    return true;
  }

  if(strcmp(key, "avg_load_store_per_cycle") == 0)
  {
    const double lmLoad = s.Get("SP", 27);
    const double lmStore = s.Get("SP", 28);
    const double gmLoad = s.Get("SP", 30);
    const double gmStore = s.Get("SP", 31);

    if(lmLoad < 0.0 || lmStore < 0.0 || gmLoad < 0.0 || gmStore < 0.0)
      return false;

    if(!QCGGPMDiv((lmLoad + lmStore + gmLoad + gmStore) * 4.0, sp, &a))
      return false;

    *out = a;
    return true;
  }

  if(strcmp(key, "texture_read_bw") == 0)
  {
    const double beatsTp = s.Get("UCHE", 4);
    const double cmpBeats = s.Get("CMP", 7);

    if(beatsTp < 0.0 || cmpBeats < 0.0 || !(dt > 0.0))
      return false;

    *out = (beatsTp / dt) * 32.0 + (cmpBeats / dt) * 32.0;
    return true;
  }

  if(strcmp(key, "vertex_read_bw") == 0 || strcmp(key, "sp_read_bw") == 0 ||
     strcmp(key, "global_buffer_read_bw") == 0 || strcmp(key, "global_image_read_bw") == 0)
  {
    const uint32_t id = strcmp(key, "vertex_read_bw") == 0
                            ? 5
                            : (strcmp(key, "sp_read_bw") == 0
                                   ? 8
                                   : (strcmp(key, "global_buffer_read_bw") == 0 ? 8 : 4));
    const double beats = s.Get("UCHE", id);

    if(beats < 0.0 || !(dt > 0.0))
      return false;

    *out = (beats / dt) * 32.0;
    return true;
  }

  if(strcmp(key, "global_buffer_read_req_bw") == 0 || strcmp(key, "global_image_read_req_bw") == 0)
  {
    const uint32_t id = strcmp(key, "global_buffer_read_req_bw") == 0 ? 13 : 9;
    const double req = s.Get("UCHE", id);

    if(req < 0.0 || !(dt > 0.0))
      return false;

    *out = (req * 16.0) / dt;
    return true;
  }

  if(strcmp(key, "gpu_bytes_write_requested") == 0)
  {
    const double req = s.Get("UCHE", 15);
    if(req < 0.0)
      return false;

    *out = req * 16.0;
    return true;
  }

  if(strcmp(key, "gpu_bytes_written") == 0)
  {
    const double evicts = s.Get("UCHE", 18);
    if(evicts < 0.0)
      return false;

    *out = evicts * 64.0;
    return true;
  }

  if(strcmp(key, "preemptions_per_second") == 0)
  {
    const double n = s.Get("CP", 3);
    if(n < 0.0 || !(dt > 0.0))
      return false;

    *out = (n * 0.5) / dt;
    return true;
  }

  if(strcmp(key, "avg_preemption_delay") == 0 || strcmp(key, "avg_preemption_save") == 0 ||
     strcmp(key, "avg_preemption_restore") == 0)
  {
    const uint32_t id = strcmp(key, "avg_preemption_delay") == 0
                            ? 4
                            : (strcmp(key, "avg_preemption_save") == 0 ? 5 : 6);
    const double cycles = s.Get("CP", id);
    const double count = s.Get("CP", 3);
    const double alwaysCount = s.Get("CP", 0);
    const double alwaysOn = s.Get("ALWAYSON", 0);

    if(cycles < 0.0 || !(count > 0.0) || !(alwaysCount > 0.0) || !(alwaysOn > 0.0))
      return false;

    double usPerTick;
    if(!QCGGPMDiv(alwaysCount / 1000.0, alwaysOn / 19200.0, &usPerTick) || !(usPerTick > 0.0))
      return false;

    *out = ((cycles + cycles) / count) / usPerTick;
    return true;
  }

  if(strcmp(key, "local_memory_load_instructions") == 0 ||
     strcmp(key, "local_memory_store_instructions") == 0 ||
     strcmp(key, "local_memory_atomic_instructions") == 0 ||
     strcmp(key, "global_memory_load_instructions") == 0 ||
     strcmp(key, "global_memory_store_instructions") == 0 ||
     strcmp(key, "global_memory_atomic_instructions") == 0)
  {
    // The GGPM getComputedValue bodies multiply the wave level counter by 4.0.
    static const struct
    {
      const char *key;
      uint32_t id;
    } kInstructionMetrics[] = {
        {"local_memory_load_instructions", 27},   {"local_memory_store_instructions", 28},
        {"local_memory_atomic_instructions", 29}, {"global_memory_load_instructions", 30},
        {"global_memory_store_instructions", 31}, {"global_memory_atomic_instructions", 32},
    };

    for(size_t i = 0; i < sizeof(kInstructionMetrics) / sizeof(kInstructionMetrics[0]); i++)
    {
      if(strcmp(key, kInstructionMetrics[i].key) != 0)
        continue;

      const double value = s.Get("SP", kInstructionMetrics[i].id);
      if(value < 0.0)
        return false;

      *out = value * 4.0;
      return true;
    }

    return false;
  }

  if(strcmp(key, "compressed_image_read_bw") == 0)
  {
    // CMP[7] * 32 bytes over the sample window.
    const double value = s.Get("CMP", 7);
    if(value < 0.0 || !(dt > 0.0))
      return false;

    *out = value * 32.0 / dt;
    return true;
  }

  if(strcmp(key, "compute_kernel_load_cycles") == 0)
  {
    // (RBBM[6] - SP[0]/numShaderPipes) / CP[0]
    const double cpAlways = s.Get("CP", 0);
    const double pipes = QCGGPMNumShaderPipes(dev);

    if(pipes <= 0.0 || !(cpAlways > 0.0) || timebase < 0.0 || sp < 0.0)
      return false;

    *out = QCGGPMPercent((timebase - sp / pipes) / cpAlways);
    return true;
  }

  if(strcmp(key, "stalled_on_system_memory") == 0)
  {
    // (UCHE[1] / numTexturePipes) / RBBM[6]
    const double pipes = QCGGPMNumTexturePipes(dev);

    if(pipes <= 0.0 || !QCGGPMDiv(s.Get("UCHE", 1) / pipes, timebase, &a))
      return false;

    *out = QCGGPMPercent(a);
    return true;
  }

  return false;
}

// The raw counters each metric's formula reads, derived by replaying the evaluate function with
// one counter at a time lowered to the -1.0 "not measured" sentinel: a metric that changes or
// stops producing a value depends on that counter. Recorded here rather than re-derived at
// runtime so a metric can be advertised as available only when everything it reads is
// addressable, and so a sampling pass selects exactly the counters its metrics need.

struct QCGGPMDependency
{
  const char *metric;
  const char *block;
  uint32_t id;
};

static const QCGGPMDependency kQCGGPMDeps[] = {
    // gpu_utilization
    {"gpu_utilization", "RBBM", 6},
    // gpu_busy
    {"gpu_busy", "RBBM", 6}, {"gpu_busy", "CP", 0},
    // cp_overhead
    {"cp_overhead", "CP", 0}, {"cp_overhead", "CP", 1},
    // vertex_fetch_stall
    {"vertex_fetch_stall", "RBBM", 6}, {"vertex_fetch_stall", "PC", 2},
    // texture_fetch_stall
    {"texture_fetch_stall", "RBBM", 6}, {"texture_fetch_stall", "SP", 4},
    // icache_miss
    {"icache_miss", "SP", 51}, {"icache_miss", "SP", 52},
    // shaders_busy
    {"shaders_busy", "RBBM", 6}, {"shaders_busy", "TP", 0}, {"shaders_busy", "SP", 0},
    // shaders_stalled
    {"shaders_stalled", "RBBM", 6}, {"shaders_stalled", "SP", 7},
    // texture_pipes_busy
    {"texture_pipes_busy", "RBBM", 6}, {"texture_pipes_busy", "TP", 0},
    // shader_alu_capacity
    {"shader_alu_capacity", "SP", 0}, {"shader_alu_capacity", "SP", 35}, {"shader_alu_capacity", "SP", 40}, {"shader_alu_capacity", "SP", 41},
    // kernel_alu_capacity
    {"kernel_alu_capacity", "SP", 0}, {"kernel_alu_capacity", "SP", 40}, {"kernel_alu_capacity", "SP", 41},
    // load_store_utilization
    {"load_store_utilization", "SP", 0}, {"load_store_utilization", "SP", 63},
    // time_alus_working
    {"time_alus_working", "SP", 0}, {"time_alus_working", "SP", 1},
    // time_efus_working
    {"time_efus_working", "SP", 0}, {"time_efus_working", "SP", 2},
    // wave_context_occupancy
    {"wave_context_occupancy", "SP", 8}, {"wave_context_occupancy", "SP", 9},
    // time_shading_fragments
    {"time_shading_fragments", "SP", 74}, {"time_shading_fragments", "SP", 76}, {"time_shading_fragments", "SP", 78},
    // time_shading_vertices
    {"time_shading_vertices", "SP", 74}, {"time_shading_vertices", "SP", 76}, {"time_shading_vertices", "SP", 78},
    // time_compute
    {"time_compute", "SP", 74}, {"time_compute", "SP", 76}, {"time_compute", "SP", 78},
    // l1_hit
    {"l1_hit", "TP", 6}, {"l1_hit", "TP", 7},
    // texture_l1_miss
    {"texture_l1_miss", "TP", 6}, {"texture_l1_miss", "TP", 7},
    // texture_l2_miss
    {"texture_l2_miss", "UCHE", 4}, {"texture_l2_miss", "UCHE", 9},
    // nearest_filtered
    {"nearest_filtered", "TP", 10}, {"nearest_filtered", "TP", 25},
    // linear_filtered
    {"linear_filtered", "TP", 10}, {"linear_filtered", "TP", 26},
    // anisotropic_filtered
    {"anisotropic_filtered", "TP", 10}, {"anisotropic_filtered", "TP", 28},
    // non_base_level
    {"non_base_level", "TP", 10}, {"non_base_level", "TP", 29},
    // prims_trivially_rejected
    {"prims_trivially_rejected", "TSE", 6}, {"prims_trivially_rejected", "TSE", 8},
    // prims_clipped
    {"prims_clipped", "TSE", 6}, {"prims_clipped", "TSE", 9},
    // global_buffer_read_l2_hit
    {"global_buffer_read_l2_hit", "UCHE", 8}, {"global_buffer_read_l2_hit", "UCHE", 13},
    // global_buffer_write_l2_hit
    {"global_buffer_write_l2_hit", "UCHE", 15}, {"global_buffer_write_l2_hit", "UCHE", 18},
    // image_read_l2_hit
    {"image_read_l2_hit", "UCHE", 4}, {"image_read_l2_hit", "UCHE", 9},
    // avg_memory_latency
    {"avg_memory_latency", "UCHE", 2}, {"avg_memory_latency", "UCHE", 3}, {"avg_memory_latency", "UCHE", 34}, {"avg_memory_latency", "UCHE", 35},
    // avg_bytes_per_vertex
    {"avg_bytes_per_vertex", "PC", 26}, {"avg_bytes_per_vertex", "UCHE", 5},
    // avg_load_store_per_cycle
    {"avg_load_store_per_cycle", "SP", 0}, {"avg_load_store_per_cycle", "SP", 27}, {"avg_load_store_per_cycle", "SP", 28}, {"avg_load_store_per_cycle", "SP", 30},
    {"avg_load_store_per_cycle", "SP", 31},
    // texture_read_bw
    {"texture_read_bw", "UCHE", 4}, {"texture_read_bw", "CMP", 7},
    // vertex_read_bw
    {"vertex_read_bw", "UCHE", 5},
    // sp_read_bw
    {"sp_read_bw", "UCHE", 8},
    // global_buffer_read_bw
    {"global_buffer_read_bw", "UCHE", 8},
    // global_buffer_read_req_bw
    {"global_buffer_read_req_bw", "UCHE", 13},
    // global_image_read_bw
    {"global_image_read_bw", "UCHE", 4},
    // global_image_read_req_bw
    {"global_image_read_req_bw", "UCHE", 9},
    // gpu_bytes_write_requested
    {"gpu_bytes_write_requested", "UCHE", 15},
    // gpu_bytes_written
    {"gpu_bytes_written", "UCHE", 18},
    // preemptions_per_second
    {"preemptions_per_second", "CP", 3},
    // avg_preemption_delay
    {"avg_preemption_delay", "CP", 0}, {"avg_preemption_delay", "CP", 3}, {"avg_preemption_delay", "CP", 4}, {"avg_preemption_delay", "ALWAYSON", 0},
    // avg_preemption_save
    {"avg_preemption_save", "CP", 0}, {"avg_preemption_save", "CP", 3}, {"avg_preemption_save", "CP", 5}, {"avg_preemption_save", "ALWAYSON", 0},
    // avg_preemption_restore
    {"avg_preemption_restore", "CP", 0}, {"avg_preemption_restore", "CP", 3}, {"avg_preemption_restore", "CP", 6}, {"avg_preemption_restore", "ALWAYSON", 0},
    // local_memory_load_instructions
    {"local_memory_load_instructions", "SP", 27},
    // local_memory_store_instructions
    {"local_memory_store_instructions", "SP", 28},
    // local_memory_atomic_instructions
    {"local_memory_atomic_instructions", "SP", 29},
    // global_memory_load_instructions
    {"global_memory_load_instructions", "SP", 30},
    // global_memory_store_instructions
    {"global_memory_store_instructions", "SP", 31},
    // global_memory_atomic_instructions
    {"global_memory_atomic_instructions", "SP", 32},
    // compressed_image_read_bw
    {"compressed_image_read_bw", "CMP", 7},
    // compute_kernel_load_cycles
    {"compute_kernel_load_cycles", "RBBM", 6}, {"compute_kernel_load_cycles", "CP", 0}, {"compute_kernel_load_cycles", "SP", 0},
    // stalled_on_system_memory
    {"stalled_on_system_memory", "RBBM", 6}, {"stalled_on_system_memory", "UCHE", 1},
};

static const uint32_t kQCGGPMDepCount = sizeof(kQCGGPMDeps) / sizeof(kQCGGPMDeps[0]);

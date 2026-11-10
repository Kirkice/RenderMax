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

// Planning and record decoding for the Adreno GGPM counter set.
//
// qc_ggpm_metrics.h holds the metric table and the formulas; this header holds the part that
// decides *what to ask the driver for* and *how to read the answer back*. Both are arithmetic over
// tables, with no GL, EGL or OS dependency, so they are kept out of qc_counters.cpp:
//
//   * the Android build compiles qc_counters.cpp, which owns the GL perf-monitor plumbing and calls
//     into this header;
//   * the desktop build compiles qc_counters_stub.cpp, which has no perf-monitor plumbing at all but
//     still compiles and runs QCGGPMPlanSelfCheck() through this same header - so a plan or decode
//     regression shows up in a desktop build and in the tests, not only on a phone.
//
// A previous round had a second, hand-mirrored copy of the pass planning in a device harness, and
// the two drifted; that is exactly what this file exists to prevent.

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "qc_ggpm_metrics.h"

// GL_AMD_performance_monitor value types the result records distinguish. Repeated rather than
// included from a GL header: this file is also compiled for the Vulkan backend and for the desktop
// build, neither of which has a GL header to include here.
enum
{
  QCGGPM_TYPE_UNSIGNED_INT64 = 0x8BC2,
  QCGGPM_TYPE_PERCENTAGE = 0x8BC3,
  QCGGPM_TYPE_UNSIGNED_INT = 0x1405,
  QCGGPM_TYPE_FLOAT = 0x1406,
};

// One raw counter as the driver addresses it. `maxActive` is the block's own limit on how many of
// its counters can be selected at once, which is what the multi-pass layout is built around;
// `type` is the GL value type of its result record.
struct QCGGPMPlanNeed
{
  uint32_t group = 0;
  uint32_t counter = 0;
  uint32_t maxActive = 0;
  uint32_t type = 0;
  bool resolved = false;
};

// A stable id for a hardware block name. Passes are split per block, so the plan needs to compare
// blocks; the recovered tables name them ("SP", "UCHE") and only their order is incidental, so the
// name is hashed rather than any position.
static inline uint32_t QCGGPMBlockId(const char *block)
{
  uint32_t hash = 2166136261u;
  for(const char *p = block; p != NULL && *p; p++)
  {
    hash ^= uint8_t(*p);
    hash *= 16777619u;
  }
  return hash;
}

// Indices into kQCGGPMNeeds of every raw counter `key` reads on a part with this device id.
//
// kQCGGPMDeps holds the counters every part shares; kQCGGPMScopedDeps holds the ones behind
// A6xCustomCountable's aggregates, which are selected by core::DeviceID and therefore only count for
// the parts that actually read them. A -1 marks a pair the metric reads but the recovered profile
// cannot name, which has to make the metric unavailable instead of quietly partial.
static inline void QCGGPMMetricNeeds(const char *key, uint32_t deviceId, std::vector<int> &out)
{
  out.clear();

  for(uint32_t d = 0; d < kQCGGPMDepCount; d++)
  {
    if(strcmp(kQCGGPMDeps[d].metric, key) != 0)
      continue;

    const int needIndex = QCGGPMSample::IndexOf(kQCGGPMDeps[d].block, kQCGGPMDeps[d].id);

    if(needIndex < 0)
      out.push_back(-1);
    else if(std::find(out.begin(), out.end(), needIndex) == out.end())
      out.push_back(needIndex);
  }

  for(uint32_t d = 0; d < kQCGGPMScopedDepCount; d++)
  {
    if(strcmp(kQCGGPMScopedDeps[d].metric, key) != 0)
      continue;

    if(!QCGGPMDepScopeApplies(kQCGGPMScopedDeps[d].scope, deviceId))
      continue;

    const int needIndex =
        QCGGPMSample::IndexOf(kQCGGPMScopedDeps[d].block, kQCGGPMScopedDeps[d].id);

    if(needIndex < 0)
      out.push_back(-1);
    else if(std::find(out.begin(), out.end(), needIndex) == out.end())
      out.push_back(needIndex);
  }
}

// Map a metric key into the Qualcomm public-ID range. Deriving the ID from the name rather than from
// the position in the table keeps a counter's GPUCounter value stable across Adreno generations and
// across versions, even though the set of metrics a given part can evaluate differs.
static inline uint32_t QCGGPMStableId(const char *key, uint32_t firstId, uint32_t lastId)
{
  const uint32_t range = lastId - firstId + 1;

  uint32_t hash = 2166136261u;
  for(const char *p = key; p != NULL && *p; p++)
  {
    hash ^= uint8_t(*p);
    hash *= 16777619u;
  }

  return firstId + (range > 0 ? (hash % range) : 0);
}

// Split the counters to measure into passes.
//
// `needed` are the counters the enabled metrics read that the driver accepts, `dedicated` are the
// ones it refused in combination and which therefore get a pass of their own. `enabledMetrics` are
// indices into kQCGGPMMetrics, used only to rank the single-pass order.
//
// Why the two layouts exist at all:
//   * A hardware block only accepts so many of its counters at once (`maxActive`), and the driver
//     silently drops the whole block's selection when the limit is exceeded - no GL error, no
//     results, so the affected metrics read as unavailable rather than wrong.
//   * Adreno exposes exactly one useful performance monitor per context: a monitor created after
//     another one in the same context reports no result for any window. Splitting into several
//     passes therefore measures nothing beyond the first pass on this driver, which is why the
//     single-pass layout is the default and the multi-pass one is opt-in.
static inline void QCGGPMPlanPasses(const std::vector<uint32_t> &needed,
                                    const std::vector<uint32_t> &dedicated,
                                    const QCGGPMPlanNeed *needs, size_t needCount,
                                    const std::vector<uint32_t> &enabledMetrics, uint32_t deviceId,
                                    bool multiPass, std::vector<std::vector<uint32_t>> &passes)
{
  passes.clear();

  if(multiPass)
  {
    // Walk the counters in the order the caller built (block by block) and start a new pass
    // whenever a block's budget would be exceeded.
    std::vector<std::pair<uint32_t, uint32_t>> usedInPass;
    std::vector<uint32_t> current;

    for(size_t i = 0; i < needed.size(); i++)
    {
      const uint32_t needIndex = needed[i];
      const uint32_t group = needIndex < needCount ? needs[needIndex].group : 0;
      const uint32_t limit = (needIndex < needCount && needs[needIndex].maxActive > 0)
                                 ? needs[needIndex].maxActive
                                 : 1;

      uint32_t used = 0;
      for(size_t u = 0; u < usedInPass.size(); u++)
      {
        if(usedInPass[u].first == group)
          used = usedInPass[u].second;
      }

      if(used + 1 > limit)
      {
        passes.push_back(current);
        current.clear();
        usedInPass.clear();
      }

      bool found = false;
      for(size_t u = 0; u < usedInPass.size(); u++)
      {
        if(usedInPass[u].first == group)
        {
          usedInPass[u].second++;
          found = true;
        }
      }
      if(!found)
        usedInPass.push_back(std::make_pair(group, 1u));

      current.push_back(needIndex);
    }

    if(!current.empty())
      passes.push_back(current);

    // Counters the driver will not select next to the others come last, each alone.
    for(size_t i = 0; i < dedicated.size(); i++)
    {
      std::vector<uint32_t> single;
      single.push_back(dedicated[i]);
      passes.push_back(single);
    }

    return;
  }

  std::vector<uint32_t> all = needed;
  for(size_t i = 0; i < dedicated.size(); i++)
  {
    if(std::find(all.begin(), all.end(), dedicated[i]) == all.end())
      all.push_back(dedicated[i]);
  }

  if(all.empty())
    return;

  // Everything in one pass, ordered so that a block's counters stay adjacent and, inside a block,
  // the counters the most enabled metrics read come first: when the driver trims the selection to
  // its block capacity, that ordering loses the fewest metrics instead of whatever happened to be
  // listed last.
  std::vector<uint32_t> order;
  std::vector<int32_t> score;

  for(size_t i = 0; i < all.size(); i++)
  {
    const uint32_t needIndex = all[i];

    int32_t uses = 0;
    for(size_t m = 0; m < enabledMetrics.size(); m++)
    {
      if(enabledMetrics[m] >= kQCGGPMMetricCount)
        continue;

      std::vector<int> deps;
      QCGGPMMetricNeeds(kQCGGPMMetrics[enabledMetrics[m]].key, deviceId, deps);
      for(size_t d = 0; d < deps.size(); d++)
      {
        if(deps[d] == int(needIndex))
        {
          uses++;
          break;
        }
      }
    }

    size_t insertAt = order.size();
    while(insertAt > 0)
    {
      const uint32_t prev = order[insertAt - 1];
      const uint32_t prevGroup = prev < needCount ? needs[prev].group : 0;
      const uint32_t group = needIndex < needCount ? needs[needIndex].group : 0;

      if(prevGroup != group || score[insertAt - 1] >= uses)
        break;
      insertAt--;
    }

    order.insert(order.begin() + insertAt, needIndex);
    score.insert(score.begin() + insertAt, uses);
  }

  passes.push_back(order);
}

// Decode a `glGetPerfMonitorCounterDataAMD` result buffer.
//
// AMD_performance_monitor records are {group, counter, value}: the value occupies one word or two
// according to GL_COUNTER_TYPE_AMD, not an inline type/instance header. In particular
// {group, counter, low, high} is the standard uint64 representation even when high is zero. The
// whole buffer is validated before anything is published, so a truncated or unknown record cannot
// leave a partially valid sample behind and be read as a real measurement.
//
// `out` has one slot per entry in kQCGGPMNeeds and is indexed by need, which is how the caller's
// sample array is laid out. Returns false when the buffer is malformed or decoded nothing.
static inline bool QCGGPMDecodeResults(const uint32_t *words, size_t wordCount,
                                       const QCGGPMPlanNeed *needs, size_t needCount,
                                       const uint32_t *needIndices, size_t needIndexCount,
                                       double *out)
{
  if(words == NULL || out == NULL)
    return false;

  std::vector<uint32_t> decodedNeeds;
  std::vector<double> decodedValues;

  size_t offset = 0;
  while(offset < wordCount)
  {
    if(wordCount - offset < 3)
      return false;

    uint32_t need = ~0U;
    for(size_t i = 0; i < needIndexCount; i++)
    {
      const uint32_t index = needIndices[i];
      if(index < needCount && needs[index].group == words[offset] &&
         needs[index].counter == words[offset + 1])
      {
        need = index;
        break;
      }
    }

    if(need == ~0U ||
       std::find(decodedNeeds.begin(), decodedNeeds.end(), need) != decodedNeeds.end())
      return false;

    const uint32_t type = needs[need].type;
    offset += 2;

    double value = 0.0;
    if(type == QCGGPM_TYPE_UNSIGNED_INT64)
    {
      if(wordCount - offset < 2)
        return false;
      uint64_t raw = 0;
      memcpy(&raw, &words[offset], sizeof(raw));
      value = double(raw);
      offset += 2;
    }
    else if(type == QCGGPM_TYPE_FLOAT || type == QCGGPM_TYPE_PERCENTAGE)
    {
      float raw = 0.0f;
      memcpy(&raw, &words[offset++], sizeof(raw));
      value = double(raw);
    }
    else if(type == QCGGPM_TYPE_UNSIGNED_INT)
    {
      value = double(words[offset++]);
    }
    else
    {
      return false;
    }

    decodedNeeds.push_back(need);
    decodedValues.push_back(value);
  }

  for(size_t i = 0; i < decodedNeeds.size(); i++)
    out[decodedNeeds[i]] = decodedValues[i];

  return !decodedNeeds.empty();
}

// Check the planning and decoding above against the recovered tables. Returns 0 when everything
// holds, or a positive count of failures with `error` describing the first one.
//
// This exists so a desktop build (and the tests) executes the same planning code the phone does,
// instead of only type-checking it: the failure modes here - a metric whose counters cannot be
// named, a need planned twice or dropped, a decode that misreads a uint64 record - are silent on a
// device and would otherwise be found only by noticing a metric that reads as unavailable.
static inline int QCGGPMPlanSelfCheck(uint32_t deviceId, char *error, size_t errorLen)
{
  int failures = 0;
  std::vector<uint32_t> allNeeds;
  std::vector<uint32_t> enabledMetrics;

  QCGGPMPlanNeed needs[kQCGGPMNeedCount];
  for(uint32_t i = 0; i < kQCGGPMNeedCount; i++)
  {
    needs[i].group = QCGGPMBlockId(kQCGGPMNeeds[i].block);
    needs[i].counter = kQCGGPMNeeds[i].id;
    needs[i].maxActive = 32;
    needs[i].type = QCGGPM_TYPE_UNSIGNED_INT;
    needs[i].resolved = true;
  }

  // Every metric has to name at least one counter, and every counter it names has to exist in the
  // need table for this part. A -1 here is a formula reading a counter the recovered profile cannot
  // address, which makes that metric permanently unavailable.
  for(uint32_t m = 0; m < kQCGGPMMetricCount; m++)
  {
    std::vector<int> deps;
    QCGGPMMetricNeeds(kQCGGPMMetrics[m].key, deviceId, deps);

    enabledMetrics.push_back(m);

    if(deps.empty())
    {
      failures++;
      if(failures == 1 && error != NULL)
        snprintf(error, errorLen, "metric %s reads no counter", kQCGGPMMetrics[m].key);
      continue;
    }

    for(size_t d = 0; d < deps.size(); d++)
    {
      if(deps[d] < 0 || deps[d] >= int(kQCGGPMNeedCount))
      {
        failures++;
        if(failures == 1 && error != NULL)
          snprintf(error, errorLen, "metric %s reads unnamed counter %d", kQCGGPMMetrics[m].key,
                   deps[d]);
        continue;
      }

      if(std::find(allNeeds.begin(), allNeeds.end(), uint32_t(deps[d])) == allNeeds.end())
        allNeeds.push_back(uint32_t(deps[d]));
    }
  }

  // Single pass: every need appears exactly once.
  {
    std::vector<std::vector<uint32_t>> passes;
    QCGGPMPlanPasses(allNeeds, std::vector<uint32_t>(), needs, kQCGGPMNeedCount, enabledMetrics,
                     deviceId, false, passes);

    if(passes.size() != 1)
    {
      failures++;
      if(failures == 1 && error != NULL)
        snprintf(error, errorLen, "single pass layout produced %u passes", uint32_t(passes.size()));
    }
    else
    {
      if(passes[0].size() != allNeeds.size())
      {
        failures++;
        if(failures == 1 && error != NULL)
          snprintf(error, errorLen, "single pass dropped %u of %u counters",
                   uint32_t(allNeeds.size() - passes[0].size()), uint32_t(allNeeds.size()));
      }

      for(size_t i = 0; i < passes[0].size(); i++)
      {
        if(std::count(passes[0].begin(), passes[0].end(), passes[0][i]) != 1)
        {
          failures++;
          if(failures == 1 && error != NULL)
            snprintf(error, errorLen, "single pass repeated counter %u", passes[0][i]);
          break;
        }
      }
    }
  }

  // Multi pass: no pass exceeds a block's own capacity, and no counter is lost.
  {
    // Give every block a capacity of three so the split has to happen even on a small need set.
    for(uint32_t i = 0; i < kQCGGPMNeedCount; i++)
      needs[i].maxActive = 3;

    std::vector<std::vector<uint32_t>> passes;
    QCGGPMPlanPasses(allNeeds, std::vector<uint32_t>(), needs, kQCGGPMNeedCount, enabledMetrics,
                     deviceId, true, passes);

    size_t planned = 0;
    for(size_t p = 0; p < passes.size(); p++)
    {
      planned += passes[p].size();

      for(uint32_t a = 0; a < passes[p].size(); a++)
      {
        uint32_t same = 0;
        for(uint32_t b = 0; b < passes[p].size(); b++)
        {
          if(needs[passes[p][a]].group == needs[passes[p][b]].group)
            same++;
        }

        if(same > 3)
        {
          failures++;
          if(failures == 1 && error != NULL)
            snprintf(error, errorLen, "multi pass put %u counters of block %u in one pass", same,
                     needs[passes[p][a]].group);
          break;
        }
      }
    }

    if(planned != allNeeds.size())
    {
      failures++;
      if(failures == 1 && error != NULL)
        snprintf(error, errorLen, "multi pass planned %u of %u counters", uint32_t(planned),
                 uint32_t(allNeeds.size()));
    }
  }

  // Decoding: one record of each value type, then a truncated buffer and a repeated record, both of
  // which must be rejected rather than partially accepted.
  {
    const uint32_t indices[3] = {0, 1, 2};
    QCGGPMPlanNeed decodeNeeds[kQCGGPMNeedCount];
    for(uint32_t i = 0; i < kQCGGPMNeedCount; i++)
      decodeNeeds[i] = needs[i];

    decodeNeeds[0].group = 100;
    decodeNeeds[0].counter = 1;
    decodeNeeds[0].type = QCGGPM_TYPE_FLOAT;
    decodeNeeds[1].group = 100;
    decodeNeeds[1].counter = 2;
    decodeNeeds[1].type = QCGGPM_TYPE_UNSIGNED_INT;
    decodeNeeds[2].group = 101;
    decodeNeeds[2].counter = 3;
    decodeNeeds[2].type = QCGGPM_TYPE_UNSIGNED_INT64;

    double values[kQCGGPMNeedCount];
    for(uint32_t i = 0; i < kQCGGPMNeedCount; i++)
      values[i] = -1.0;

    // A uint64 record is four words; the buffer is built the way the driver lays it out.
    uint32_t buffer[10] = {100, 1, 0, 100, 2, 7, 101, 3, 0, 0};
    float asFloat = 2.5f;
    memcpy(&buffer[2], &asFloat, sizeof(asFloat));
    const uint64_t big = 0x0000000100000002ull;
    memcpy(&buffer[8], &big, sizeof(big));

    if(!QCGGPMDecodeResults(buffer, 10, decodeNeeds, kQCGGPMNeedCount, indices, 3, values))
    {
      failures++;
      if(failures == 1 && error != NULL)
        snprintf(error, errorLen, "decode rejected a well formed buffer");
    }
    else if(values[0] != 2.5 || values[1] != 7.0 || values[2] != double(big))
    {
      failures++;
      if(failures == 1 && error != NULL)
        snprintf(error, errorLen, "decode produced %g/%g/%g", values[0], values[1], values[2]);
    }

    if(QCGGPMDecodeResults(buffer, 9, decodeNeeds, kQCGGPMNeedCount, indices, 3, values))
    {
      failures++;
      if(failures == 1 && error != NULL)
        snprintf(error, errorLen, "decode accepted a truncated uint64 record");
    }

    const uint32_t repeated[6] = {100, 1, 0, 100, 1, 0};
    if(QCGGPMDecodeResults(repeated, 6, decodeNeeds, kQCGGPMNeedCount, indices, 3, values))
    {
      failures++;
      if(failures == 1 && error != NULL)
        snprintf(error, errorLen, "decode accepted the same counter twice");
    }
  }

  return failures;
}

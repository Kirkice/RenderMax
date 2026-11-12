/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2017-2026 Baldur Karlsson
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

#include "StatisticsViewer.h"
#include <QFontDatabase>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QScrollBar>
#include <QPushButton>
#include <QPointer>
#include <QResizeEvent>
#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>
#include "Code/QRDUtils.h"
#include "Code/SystemTrace.h"
#include "ui_StatisticsViewer.h"

static const int HistogramWidth = 128;
static const QString Stars = QString(HistogramWidth, QLatin1Char('*'));

QString Pow2IndexAsReadable(int index)
{
  uint64_t value = 1ULL << index;

  if(value >= (1024 * 1024))
  {
    float slice = (float)value / (1024 * 1024);
    return QFormatStr("%1MB").arg(Formatter::Format(slice));
  }
  else if(value >= 1024)
  {
    float slice = (float)value / 1024;
    return QFormatStr("%1KB").arg(Formatter::Format(slice));
  }
  else
  {
    return QFormatStr("%1B").arg(Formatter::Format((float)value));
  }
}

int SliceForString(const QString &s, uint32_t value, uint32_t maximum)
{
  if(value == 0 || maximum == 0)
    return 0;

  float ratio = (float)value / maximum;
  int slice = (int)(ratio * s.length());
  return qMax(1, slice);
}

QString CountOrEmpty(uint32_t count)
{
  if(count == 0)
    return QString();
  else
    return QFormatStr("(%1)").arg(count);
}

QString CreateSimpleIntegerHistogram(const QString &legend, const rdcarray<uint32_t> &array)
{
  uint32_t maxCount = 0;
  int maxWithValue = 0;

  for(int o = 0; o < array.count(); o++)
  {
    uint32_t value = array[o];
    if(value > 0)
      maxWithValue = o;
    maxCount = qMax(maxCount, value);
  }

  QString text = QFormatStr("\n%1:\n").arg(legend);

  for(int o = 0; o <= maxWithValue; o++)
  {
    uint32_t count = array[o];
    int slice = SliceForString(Stars, count, maxCount);
    text += QFormatStr("%1: %2 %3\n").arg(o, 4).arg(Stars.left(slice)).arg(CountOrEmpty(count));
  }

  return text;
}

void StatisticsViewer::AppendDrawStatistics()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  // #mivance see AppendConstantBindStatistics
  const DrawcallStats &draws = frameInfo.stats.draws;

  m_Report.append(tr("\n*** Draw Statistics ***\n\n"));

  m_Report.append(tr("Total calls: %1, instanced: %2, indirect: %3\n")
                      .arg(draws.calls)
                      .arg(draws.instanced)
                      .arg(draws.indirect));

  if(draws.instanced > 0)
  {
    m_Report.append(tr("\nInstance counts:\n"));
    uint32_t maxCount = 0;
    int maxWithValue = 0;
    int maximum = draws.counts.count();
    for(int s = 1; s < maximum; s++)
    {
      uint32_t value = draws.counts[s];
      if(value > 0)
        maxWithValue = s;
      maxCount = qMax(maxCount, value);
    }

    for(int s = 1; s <= maxWithValue; s++)
    {
      uint32_t count = draws.counts[s];
      int slice = SliceForString(Stars, count, maxCount);
      m_Report.append(QFormatStr("%1%2: %3 %4\n")
                          .arg((s == maximum - 1) ? lit(">=") : lit("  "))
                          .arg(s, 2)
                          .arg(Stars.left(slice))
                          .arg(CountOrEmpty(count)));
    }
  }
}

void StatisticsViewer::AppendDispatchStatistics()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  m_Report.append(tr("\n*** Dispatch Statistics ***\n\n"));
  m_Report.append(tr("Total calls: %1, indirect: %2\n")
                      .arg(frameInfo.stats.dispatches.calls)
                      .arg(frameInfo.stats.dispatches.indirect));
}

void StatisticsViewer::AppendInputAssemblerStatistics()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  const IndexBindStats &indices = frameInfo.stats.indices;
  const LayoutBindStats &layouts = frameInfo.stats.layouts;

  const VertexBindStats &vertices = frameInfo.stats.vertices;

  m_Report.append(tr("\n*** Input Assembler Statistics ***\n\n"));

  m_Report.append(tr("Total index calls: %1, non-null index sets: %2, null index sets: %3\n")
                      .arg(indices.calls)
                      .arg(indices.sets)
                      .arg(indices.nulls));
  m_Report.append(tr("Total layout calls: %1, non-null layout sets: %2, null layout sets: %3\n")
                      .arg(layouts.calls)
                      .arg(layouts.sets)
                      .arg(layouts.nulls));
  m_Report.append(tr("Total vertex calls: %1, non-null vertex sets: %2, null vertex sets: %3\n")
                      .arg(vertices.calls)
                      .arg(vertices.sets)
                      .arg(vertices.nulls));

  m_Report.append(CreateSimpleIntegerHistogram(tr("Aggregate vertex slot counts per invocation"),
                                               vertices.bindslots));
}

void StatisticsViewer::AppendShaderStatistics()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  const rdcarray<ShaderChangeStats> &shaders = frameInfo.stats.shaders;
  ShaderChangeStats totalShadersPerStage;
  for(auto s : indices<ShaderStage>())
  {
    totalShadersPerStage.calls += shaders[s].calls;
    totalShadersPerStage.sets += shaders[s].sets;
    totalShadersPerStage.nulls += shaders[s].nulls;
    totalShadersPerStage.redundants += shaders[s].redundants;
  }

  m_Report.append(tr("\n*** Shader Set Statistics ***\n\n"));

  for(auto s : indices<ShaderStage>())
  {
    m_Report.append(tr("%1 calls: %2, non-null shader sets: %3, null shader sets: %4, "
                       "redundant shader sets: %5\n")
                        .arg(m_Ctx.CurPipelineState().Abbrev(StageFromIndex(s)))
                        .arg(shaders[s].calls)
                        .arg(shaders[s].sets)
                        .arg(shaders[s].nulls)
                        .arg(shaders[s].redundants));
  }

  m_Report.append(tr("Total calls: %1, non-null shader sets: %2, null shader sets: %3, "
                     "redundant shader sets: %4\n")
                      .arg(totalShadersPerStage.calls)
                      .arg(totalShadersPerStage.sets)
                      .arg(totalShadersPerStage.nulls)
                      .arg(totalShadersPerStage.redundants));
}

void StatisticsViewer::AppendConstantBindStatistics()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  // #mivance C++-side we guarantee all stages will have the same slots
  // and sizes count, so pattern off of the first frame's first stage
  const ConstantBindStats &reference = frameInfo.stats.constants[0];

  // #mivance there is probably a way to iterate the fields via
  // GetType()/GetField() and build a sort of dynamic min/max/average
  // structure for a given type with known integral types (or arrays
  // thereof), but given we're heading for a Qt/C++ rewrite of the UI
  // perhaps best not to dwell too long on that
  ConstantBindStats totalConstantsPerStage[arraydim<ShaderStage>()];
  for(auto s : indices<ShaderStage>())
  {
    totalConstantsPerStage[s].bindslots.resize(reference.bindslots.size());
    totalConstantsPerStage[s].sizes.resize(reference.sizes.size());
  }

  {
    const rdcarray<ConstantBindStats> &constants = frameInfo.stats.constants;
    for(auto s : indices<ShaderStage>())
    {
      totalConstantsPerStage[s].calls += constants[s].calls;
      totalConstantsPerStage[s].sets += constants[s].sets;
      totalConstantsPerStage[s].nulls += constants[s].nulls;

      for(int l = 0; l < constants[s].bindslots.count(); l++)
        totalConstantsPerStage[s].bindslots[l] += constants[s].bindslots[l];

      for(int z = 0; z < constants[s].sizes.count(); z++)
        totalConstantsPerStage[s].sizes[z] += constants[s].sizes[z];
    }
  }

  ConstantBindStats totalConstantsForAllStages;
  totalConstantsForAllStages.bindslots.resize(totalConstantsPerStage[0].bindslots.size());
  totalConstantsForAllStages.sizes.resize(totalConstantsPerStage[0].sizes.size());

  for(auto s : indices<ShaderStage>())
  {
    const ConstantBindStats &perStage = totalConstantsPerStage[s];
    totalConstantsForAllStages.calls += perStage.calls;
    totalConstantsForAllStages.sets += perStage.sets;
    totalConstantsForAllStages.nulls += perStage.nulls;

    for(int l = 0; l < perStage.bindslots.count(); l++)
      totalConstantsForAllStages.bindslots[l] += perStage.bindslots[l];

    for(int z = 0; z < perStage.sizes.count(); z++)
      totalConstantsForAllStages.sizes[z] += perStage.sizes[z];
  }

  m_Report.append(tr("\n*** Constant Bind Statistics ***\n\n"));

  for(auto s : indices<ShaderStage>())
  {
    m_Report.append(tr("%1 calls: %2, non-null buffer sets: %3, null buffer sets: %4\n")
                        .arg(m_Ctx.CurPipelineState().Abbrev(StageFromIndex(s)))
                        .arg(totalConstantsPerStage[s].calls)
                        .arg(totalConstantsPerStage[s].sets)
                        .arg(totalConstantsPerStage[s].nulls));
  }

  m_Report.append(tr("Total calls: %1, non-null buffer sets: %2, null buffer sets: %3\n")
                      .arg(totalConstantsForAllStages.calls)
                      .arg(totalConstantsForAllStages.sets)
                      .arg(totalConstantsForAllStages.nulls));

  m_Report.append(
      CreateSimpleIntegerHistogram(tr("Aggregate slot counts per invocation across all stages"),
                                   totalConstantsForAllStages.bindslots));

  m_Report.append(tr("\nAggregate constant buffer sizes across all stages:\n"));
  uint32_t maxCount = 0;
  int maxWithValue = 0;
  for(int s = 0; s < totalConstantsForAllStages.sizes.count(); s++)
  {
    uint32_t value = totalConstantsForAllStages.sizes[s];
    if(value > 0)
      maxWithValue = s;
    maxCount = qMax(maxCount, value);
  }

  for(int s = 0; s <= maxWithValue; s++)
  {
    uint32_t count = totalConstantsForAllStages.sizes[s];
    int slice = SliceForString(Stars, count, maxCount);
    m_Report.append(QFormatStr("%1: %2 %3\n")
                        .arg(Pow2IndexAsReadable(s), 8)
                        .arg(Stars.left(slice))
                        .arg(CountOrEmpty(count)));
  }
}

void StatisticsViewer::AppendSamplerBindStatistics()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  // #mivance see AppendConstantBindStatistics
  const SamplerBindStats &reference = frameInfo.stats.samplers[0];

  SamplerBindStats totalSamplersPerStage[arraydim<ShaderStage>()];
  for(auto s : indices<ShaderStage>())
  {
    totalSamplersPerStage[s].bindslots.resize(reference.bindslots.size());
  }

  {
    const rdcarray<SamplerBindStats> &samplers = frameInfo.stats.samplers;
    for(auto s : indices<ShaderStage>())
    {
      totalSamplersPerStage[s].calls += samplers[s].calls;
      totalSamplersPerStage[s].sets += samplers[s].sets;
      totalSamplersPerStage[s].nulls += samplers[s].nulls;

      for(int l = 0; l < samplers[s].bindslots.count(); l++)
      {
        totalSamplersPerStage[s].bindslots[l] += samplers[s].bindslots[l];
      }
    }
  }

  SamplerBindStats totalSamplersForAllStages;
  totalSamplersForAllStages.bindslots.resize(totalSamplersPerStage[0].bindslots.size());

  for(auto s : indices<ShaderStage>())
  {
    SamplerBindStats perStage = totalSamplersPerStage[s];
    totalSamplersForAllStages.calls += perStage.calls;
    totalSamplersForAllStages.sets += perStage.sets;
    totalSamplersForAllStages.nulls += perStage.nulls;
    for(int l = 0; l < perStage.bindslots.count(); l++)
    {
      totalSamplersForAllStages.bindslots[l] += perStage.bindslots[l];
    }
  }

  m_Report.append(tr("\n*** Sampler Bind Statistics ***\n\n"));

  for(auto s : indices<ShaderStage>())
  {
    m_Report.append(tr("%1 calls: %2, non-null sampler sets: %3, null sampler sets: %4\n")
                        .arg(m_Ctx.CurPipelineState().Abbrev(StageFromIndex(s)))
                        .arg(totalSamplersPerStage[s].calls)
                        .arg(totalSamplersPerStage[s].sets)
                        .arg(totalSamplersPerStage[s].nulls));
  }

  m_Report.append(tr("Total calls: %1, non-null sampler sets: %2, null sampler sets: %3\n")
                      .arg(totalSamplersForAllStages.calls)
                      .arg(totalSamplersForAllStages.sets)
                      .arg(totalSamplersForAllStages.nulls));

  m_Report.append(
      CreateSimpleIntegerHistogram(tr("Aggregate slot counts per invocation across all stages"),
                                   totalSamplersForAllStages.bindslots));
}

void StatisticsViewer::AppendResourceBindStatistics()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  // #mivance see AppendConstantBindStatistics
  const ResourceBindStats &reference = frameInfo.stats.resources[0];

  ResourceBindStats totalResourcesPerStage[arraydim<ShaderStage>()];
  for(auto s : indices<ShaderStage>())
  {
    totalResourcesPerStage[s].types.resize(reference.types.size());
    totalResourcesPerStage[s].bindslots.resize(reference.bindslots.size());
  }

  {
    const rdcarray<ResourceBindStats> &resources = frameInfo.stats.resources;
    for(auto s : indices<ShaderStage>())
    {
      totalResourcesPerStage[s].calls += resources[s].calls;
      totalResourcesPerStage[s].sets += resources[s].sets;
      totalResourcesPerStage[s].nulls += resources[s].nulls;

      for(int z = 0; z < resources[s].types.count(); z++)
      {
        totalResourcesPerStage[s].types[z] += resources[s].types[z];
      }

      for(int l = 0; l < resources[s].bindslots.count(); l++)
      {
        totalResourcesPerStage[s].bindslots[l] += resources[s].bindslots[l];
      }
    }
  }

  ResourceBindStats totalResourcesForAllStages;
  totalResourcesForAllStages.types.resize(totalResourcesPerStage[0].types.size());
  totalResourcesForAllStages.bindslots.resize(totalResourcesPerStage[0].bindslots.size());

  for(auto s : indices<ShaderStage>())
  {
    ResourceBindStats perStage = totalResourcesPerStage[s];
    totalResourcesForAllStages.calls += perStage.calls;
    totalResourcesForAllStages.sets += perStage.sets;
    totalResourcesForAllStages.nulls += perStage.nulls;
    for(int t = 0; t < perStage.types.count(); t++)
    {
      totalResourcesForAllStages.types[t] += perStage.types[t];
    }
    for(int l = 0; l < perStage.bindslots.count(); l++)
    {
      totalResourcesForAllStages.bindslots[l] += perStage.bindslots[l];
    }
  }

  m_Report.append(tr("\n*** Resource Bind Statistics ***\n\n"));

  for(auto s : indices<ShaderStage>())
  {
    m_Report.append(tr("%1 calls: %2 non-null resource sets: %3 null resource sets: %4\n")
                        .arg(m_Ctx.CurPipelineState().Abbrev(StageFromIndex(s)))
                        .arg(totalResourcesPerStage[s].calls)
                        .arg(totalResourcesPerStage[s].sets)
                        .arg(totalResourcesPerStage[s].nulls));
  }

  m_Report.append(tr("Total calls: %1 non-null resource sets: %2 null resource sets: %3\n")
                      .arg(totalResourcesForAllStages.calls)
                      .arg(totalResourcesForAllStages.sets)
                      .arg(totalResourcesForAllStages.nulls));

  uint32_t maxCount = 0;
  int maxWithCount = 0;

  m_Report.append(tr("\nResource types across all stages:\n"));
  for(int s = 0; s < totalResourcesForAllStages.types.count(); s++)
  {
    uint32_t count = totalResourcesForAllStages.types[s];
    if(count > 0)
      maxWithCount = s;
    maxCount = qMax(maxCount, count);
  }

  for(int s = 0; s <= maxWithCount; s++)
  {
    uint32_t count = totalResourcesForAllStages.types[s];
    int slice = SliceForString(Stars, count, maxCount);
    TextureType type = (TextureType)s;
    m_Report.append(
        QFormatStr("%1: %2 %3\n").arg(ToQStr(type), 20).arg(Stars.left(slice)).arg(CountOrEmpty(count)));
  }

  m_Report.append(
      CreateSimpleIntegerHistogram(tr("Aggregate slot counts per invocation across all stages"),
                                   totalResourcesForAllStages.bindslots));
}

void StatisticsViewer::AppendUpdateStatistics()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  // #mivance see AppendConstantBindStatistics
  const ResourceUpdateStats &reference = frameInfo.stats.updates;

  ResourceUpdateStats totalUpdates;
  totalUpdates.types.resize(reference.types.size());
  totalUpdates.sizes.resize(reference.sizes.size());

  {
    ResourceUpdateStats updates = frameInfo.stats.updates;

    totalUpdates.calls += updates.calls;
    totalUpdates.clients += updates.clients;
    totalUpdates.servers += updates.servers;

    for(int t = 0; t < updates.types.count(); t++)
      totalUpdates.types[t] += updates.types[t];

    for(int t = 0; t < updates.sizes.count(); t++)
      totalUpdates.sizes[t] += updates.sizes[t];
  }

  m_Report.append(tr("\n*** Resource Update Statistics ***\n\n"));

  m_Report.append(tr("Total calls: %1, client-updated memory: %2, server-updated memory: %3\n")
                      .arg(totalUpdates.calls)
                      .arg(totalUpdates.clients)
                      .arg(totalUpdates.servers));

  m_Report.append(tr("\nUpdated resource types:\n"));
  uint32_t maxCount = 0;
  int maxWithValue = 0;
  for(int s = 1; s < totalUpdates.types.count(); s++)
  {
    uint32_t value = totalUpdates.types[s];
    if(value > 0)
      maxWithValue = s;
    maxCount = qMax(maxCount, value);
  }

  for(int s = 1; s <= maxWithValue; s++)
  {
    uint32_t count = totalUpdates.types[s];
    int slice = SliceForString(Stars, count, maxCount);
    TextureType type = (TextureType)s;
    m_Report.append(
        QFormatStr("%1: %2 %3\n").arg(ToQStr(type), 20).arg(Stars.left(slice)).arg(CountOrEmpty(count)));
  }

  m_Report.append(tr("\nUpdated resource sizes:\n"));
  maxCount = 0;
  maxWithValue = 0;
  for(int s = 0; s < totalUpdates.sizes.count(); s++)
  {
    uint32_t value = totalUpdates.sizes[s];
    if(value > 0)
      maxWithValue = s;
    maxCount = qMax(maxCount, value);
  }

  for(int s = 0; s <= maxWithValue; s++)
  {
    uint32_t count = totalUpdates.sizes[s];
    int slice = SliceForString(Stars, count, maxCount);
    m_Report.append(QFormatStr("%1: %2 %3\n")
                        .arg(Pow2IndexAsReadable(s), 8)
                        .arg(Stars.left(slice))
                        .arg(CountOrEmpty(count)));
  }
}

void StatisticsViewer::AppendBlendStatistics()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  BlendStats blends = frameInfo.stats.blends;
  m_Report.append(tr("\n*** Blend Statistics ***\n"));
  m_Report.append(
      tr("Blend calls: %1 non-null sets: %2, null (default) sets: %3, redundant sets: %4\n")
          .arg(blends.calls)
          .arg(blends.sets)
          .arg(blends.nulls)
          .arg(blends.redundants));
}

void StatisticsViewer::AppendDepthStencilStatistics()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  DepthStencilStats depths = frameInfo.stats.depths;
  m_Report.append(tr("\n*** Depth Stencil Statistics ***\n"));
  m_Report.append(tr("Depth/stencil calls: %1 non-null sets: %2, null (default) sets: "
                     "%3, redundant sets: %4\n")
                      .arg(depths.calls)
                      .arg(depths.sets)
                      .arg(depths.nulls)
                      .arg(depths.redundants));
}

void StatisticsViewer::AppendRasterizationStatistics()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  RasterizationStats rasters = frameInfo.stats.rasters;
  m_Report.append(tr("\n*** Rasterization Statistics ***\n"));
  m_Report.append(tr("Rasterization calls: %1 non-null sets: %2, null (default) sets: "
                     "%3, redundant sets: %4\n")
                      .arg(rasters.calls)
                      .arg(rasters.sets)
                      .arg(rasters.nulls)
                      .arg(rasters.redundants));
  m_Report.append(CreateSimpleIntegerHistogram(tr("Viewports set"), rasters.viewports));
  m_Report.append(CreateSimpleIntegerHistogram(tr("Scissors set"), rasters.rects));
}

void StatisticsViewer::AppendOutputStatistics()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  OutputTargetStats outputs = frameInfo.stats.outputs;
  m_Report.append(tr("\n*** Output Statistics ***\n"));
  m_Report.append(tr("Output calls: %1 non-null sets: %2, null sets: %3\n")
                      .arg(outputs.calls)
                      .arg(outputs.sets)
                      .arg(outputs.nulls));
  m_Report.append(CreateSimpleIntegerHistogram(tr("Outputs set"), outputs.bindslots));
}

void StatisticsViewer::AppendDetailedInformation()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  if(!frameInfo.stats.recorded)
    return;

  AppendDrawStatistics();
  AppendDispatchStatistics();
  AppendInputAssemblerStatistics();
  AppendShaderStatistics();
  AppendConstantBindStatistics();
  AppendSamplerBindStatistics();
  AppendResourceBindStatistics();
  AppendBlendStatistics();
  AppendDepthStencilStatistics();
  AppendRasterizationStatistics();
  AppendUpdateStatistics();
  AppendOutputStatistics();
}

void StatisticsViewer::CountContributingEvents(const ActionDescription &action, uint32_t &drawCount,
                                               uint32_t &dispatchCount, uint32_t &diagnosticCount)
{
  const ActionFlags diagnosticMask =
      ActionFlags::SetMarker | ActionFlags::PushMarker | ActionFlags::PopMarker;
  ActionFlags diagnosticMasked = action.flags & diagnosticMask;

  if(diagnosticMasked != ActionFlags::NoFlags)
    diagnosticCount += 1;

  if(action.flags & (ActionFlags::MeshDispatch | ActionFlags::Drawcall))
    drawCount += 1;

  if(action.flags & ActionFlags::Dispatch)
    dispatchCount += 1;

  for(const ActionDescription &c : action.children)
    CountContributingEvents(c, drawCount, dispatchCount, diagnosticCount);
}

void StatisticsViewer::AppendAPICallSummary()
{
  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  if(!frameInfo.stats.recorded)
    return;

  uint32_t numConstantSets = 0;
  uint32_t numSamplerSets = 0;
  uint32_t numResourceSets = 0;
  uint32_t numShaderSets = 0;

  for(auto s : indices<ShaderStage>())
  {
    numConstantSets += frameInfo.stats.constants[s].calls;
    numSamplerSets += frameInfo.stats.samplers[s].calls;
    numResourceSets += frameInfo.stats.resources[s].calls;
    numShaderSets += frameInfo.stats.shaders[s].calls;
  }

  uint32_t numResourceUpdates = frameInfo.stats.updates.calls;
  uint32_t numIndexVertexSets = (frameInfo.stats.indices.calls + frameInfo.stats.vertices.calls +
                                 frameInfo.stats.layouts.calls);
  uint32_t numBlendSets = frameInfo.stats.blends.calls;
  uint32_t numDepthStencilSets = frameInfo.stats.depths.calls;
  uint32_t numRasterizationSets = frameInfo.stats.rasters.calls;
  uint32_t numOutputSets = frameInfo.stats.outputs.calls;

  m_Report += tr("\tIndex/vertex bind calls: %1\n").arg(numIndexVertexSets);
  m_Report += tr("\tConstant bind calls: %1\n").arg(numConstantSets);
  m_Report += tr("\tSampler bind calls: %1\n").arg(numSamplerSets);
  m_Report += tr("\tResource bind calls: %1\n").arg(numResourceSets);
  m_Report += tr("\tShader set calls: %1\n").arg(numShaderSets);
  m_Report += tr("\tBlend set calls: %1\n").arg(numBlendSets);
  m_Report += tr("\tDepth/stencil set calls: %1\n").arg(numDepthStencilSets);
  m_Report += tr("\tRasterization set calls: %1\n").arg(numRasterizationSets);
  m_Report += tr("\tResource update calls: %1\n").arg(numResourceUpdates);
  m_Report += tr("\tOutput set calls: %1\n").arg(numOutputSets);
}

void StatisticsViewer::GenerateReport()
{
  const rdcarray<ActionDescription> &curActions = m_Ctx.CurRootActions();

  uint32_t drawCount = 0;
  uint32_t dispatchCount = 0;
  uint32_t diagnosticCount = 0;
  for(const ActionDescription &action : curActions)
    CountContributingEvents(action, drawCount, dispatchCount, diagnosticCount);

  const ActionDescription *lastAction = m_Ctx.GetLastAction();
  const uint64_t excluded = uint64_t(drawCount) + dispatchCount + diagnosticCount;
  uint32_t numAPIcalls = lastAction && lastAction->eventId > excluded
                            ? uint32_t(lastAction->eventId - excluded) : 0;

  int numTextures = m_Ctx.GetTextures().count();
  int numBuffers = m_Ctx.GetBuffers().count();

  uint64_t IBBytes = 0;
  uint64_t VBBytes = 0;
  uint64_t BufBytes = 0;
  for(const BufferDescription &b : m_Ctx.GetBuffers())
  {
    BufBytes += b.length;

    if(b.creationFlags & BufferCategory::Index)
      IBBytes += b.length;
    if(b.creationFlags & BufferCategory::Vertex)
      VBBytes += b.length;
  }

  uint64_t RTBytes = 0;
  uint64_t TexBytes = 0;
  uint64_t LargeTexBytes = 0;

  int numRTs = 0;
  float texW = 0, texH = 0;
  float largeTexW = 0, largeTexH = 0;
  int texCount = 0, largeTexCount = 0;
  for(const TextureDescription &t : m_Ctx.GetTextures())
  {
    if(t.creationFlags & (TextureCategory::ColorTarget | TextureCategory::DepthTarget))
    {
      numRTs++;

      RTBytes += t.byteSize;
    }
    else
    {
      texW += (float)t.width;
      texH += (float)t.height;
      texCount++;

      TexBytes += t.byteSize;

      if(t.width > 32 && t.height > 32)
      {
        largeTexW += (float)t.width;
        largeTexH += (float)t.height;
        largeTexCount++;

        LargeTexBytes += t.byteSize;
      }
    }
  }

  if(texCount > 0)
  {
    texW /= texCount;
    texH /= texCount;
  }

  if(largeTexCount > 0)
  {
    largeTexW /= largeTexCount;
    largeTexH /= largeTexCount;
  }

  float drawRatio = 0.0f;
  if(drawCount + dispatchCount > 0)
    drawRatio = (float)numAPIcalls / (float)(drawCount + dispatchCount);

  const FrameDescription &frameInfo = m_Ctx.FrameInfo();

  float compressedMB = (float)frameInfo.compressedFileSize / (1024.0f * 1024.0f);
  float uncompressedMB = (float)frameInfo.uncompressedFileSize / (1024.0f * 1024.0f);
  float compressRatio = compressedMB > 0.0f ? uncompressedMB / compressedMB : 0.0f;
  float persistentMB = (float)frameInfo.persistentSize / (1024.0f * 1024.0f);
  float initDataMB = (float)frameInfo.initDataSize / (1024.0f * 1024.0f);

  QString header =
      tr("Stats for %1.\n\nFile size: %2MB (%3MB uncompressed, compression ratio %4:1)\n"
         "Persistent Data (approx): %5MB, Frame-initial data (approx): %6MB\n")
          .arg(QFileInfo(m_Ctx.GetCaptureFilename()).fileName())
          .arg(compressedMB, 2, 'f', 2)
          .arg(uncompressedMB, 2, 'f', 2)
          .arg(compressRatio, 2, 'f', 2)
          .arg(persistentMB, 2, 'f', 2)
          .arg(initDataMB, 2, 'f', 2);
  QString drawList = tr("Draw calls: %1\nDispatch calls: %2\n").arg(drawCount).arg(dispatchCount);
  QString ratio = tr("API:Draw/Dispatch call ratio: %1\n\n").arg(drawRatio);
  QString textures = tr("%1 Textures - %2 MB (%3 MB over 32x32), %4 RTs - %5 MB.\n"
                        "Avg. tex dimension: %6x%7 (%8x%9 over 32x32)\n")
                         .arg(numTextures)
                         .arg((float)TexBytes / (1024.0f * 1024.0f), 2, 'f', 2)
                         .arg((float)LargeTexBytes / (1024.0f * 1024.0f), 2, 'f', 2)
                         .arg(numRTs)
                         .arg((float)RTBytes / (1024.0f * 1024.0f), 2, 'f', 2)
                         .arg(texW)
                         .arg(texH)
                         .arg(largeTexW)
                         .arg(largeTexH);
  QString buffers = tr("%1 Buffers - %2 MB total %3 MB IBs %4 MB VBs.\n")
                        .arg(numBuffers)
                        .arg((float)BufBytes / (1024.0f * 1024.0f), 2, 'f', 2)
                        .arg((float)IBBytes / (1024.0f * 1024.0f), 2, 'f', 2)
                        .arg((float)VBBytes / (1024.0f * 1024.0f), 2, 'f', 2);
  QString load = tr("%1 MB - Grand total GPU buffer + texture load.\n")
                     .arg((float)(TexBytes + BufBytes + RTBytes) / (1024.0f * 1024.0f), 2, 'f', 2);

  m_Report = header;

  m_Report.append(tr("\n*** Summary ***\n\n"));
  m_Report.append(drawList);
  m_Report += tr("API calls: %1\n").arg(numAPIcalls);
  AppendAPICallSummary();
  m_Report.append(ratio);
  m_Report.append(textures);
  m_Report.append(buffers);
  m_Report.append(load);

  AppendDetailedInformation();
}

// Shared geometry and typography for the two dashboard pages. Only layout and type are customised:
// every colour is a palette role, so the page follows the application's UE theme instead of carrying
// a second theme of its own.
static QString OverviewStyleSheet()
{
  return lit(
      "QWidget#statisticsOverview, QWidget#statisticsTraceReport { background: palette(window); }"
      "QFrame[card=\"true\"] { background: palette(base); border: 1px solid palette(mid); border-radius: 14px; }"
      "QLabel { background: transparent; border: none; color: palette(window-text); }"
      "QLabel[role=\"caption\"], QLabel[role=\"metricTitle\"] { font-size: 12px; }"
      "QLabel[role=\"value\"] { font-size: 27px; font-weight: bold; }"
      "QLabel[role=\"metric\"] { font-size: 30px; font-weight: bold; }"
      "QLabel[role=\"heading\"] { font-size: 14px; font-weight: bold; }"
      "QLabel[role=\"title\"] { font-size: 28px; font-weight: bold; }"
      "QLabel[role=\"badge\"] { font-size: 11px; font-weight: bold; }"
      "QLabel[role=\"pill\"] { font-size: 10px; font-weight: bold; background: palette(mid); "
      "border-radius: 9px; padding: 2px 8px; }"
      "QProgressBar { border: 0; background: palette(window); height: 8px; border-radius: 4px; }"
      "QProgressBar::chunk { background: palette(highlight); border-radius: 4px; }"
      "QFrame[stripe=\"odd\"] { background: rgba(255, 255, 255, 7); border: 0; border-radius: 6px; }"
      "QFrame[stripe=\"even\"] { background: transparent; border: 0; border-radius: 6px; }");
}

static QLabel *OverviewLabel(const QString &text, const char *role)
{
  QLabel *result = new QLabel(text);
  result->setTextFormat(Qt::PlainText);
  result->setWordWrap(true);
  result->setMinimumWidth(0);
  result->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  result->setProperty("role", QString::fromLatin1(role));
  result->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return result;
}

static QFrame *OverviewCard()
{
  QFrame *frame = new QFrame;
  frame->setProperty("card", true);
  frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  return frame;
}

// A grid that keeps at most `maximumColumns` metric cards per row and drops to fewer columns as the
// page narrows. Qt has no responsive grid: a plain QGridLayout would either force horizontal scrolling
// or squeeze the cards into unreadable slivers once the panel is docked narrow.
class CardGrid : public QWidget
{
public:
  CardGrid(int maximumColumns, int minimumCardWidth, QWidget *parent = nullptr)
      : QWidget(parent), m_MaxColumns(maximumColumns), m_MinimumCardWidth(minimumCardWidth)
  {
    m_Grid = new QGridLayout(this);
    m_Grid->setContentsMargins(0, 0, 0, 0);
    m_Grid->setSpacing(12);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  }

  void addCard(QWidget *card)
  {
    card->setMinimumWidth(m_MinimumCardWidth);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_Cards.push_back(card);
    // Force a re-add: the cards were appended while the grid already held the previous ones.
    m_AppliedColumns = 0;
    applyLayout();
  }

protected:
  void resizeEvent(QResizeEvent *event) override
  {
    QWidget::resizeEvent(event);
    applyLayout();
  }

private:
  void applyLayout()
  {
    if(m_Cards.isEmpty())
      return;
    // Measure what actually fits once the grid's own spacing is removed, so the column count is the
    // number of cards that fit rather than an estimate that leaves the last one clipped.
    const int spacing = m_Grid->spacing();
    const int width = qMax(this->width(), m_MinimumCardWidth);
    const int columns = qBound(1, (width + spacing) / (m_MinimumCardWidth + spacing), m_MaxColumns);
    if(columns == m_AppliedColumns)
      return;
    m_AppliedColumns = columns;
    for(QWidget *card : m_Cards)
      m_Grid->removeWidget(card);
    for(int i = 0; i < m_MaxColumns; i++)
      m_Grid->setColumnStretch(i, i < columns ? 1 : 0);
    for(int i = 0; i < m_Cards.size(); i++)
      m_Grid->addWidget(m_Cards[i], i / columns, i % columns);
  }

  QGridLayout *m_Grid = nullptr;
  QVector<QWidget *> m_Cards;
  int m_MaxColumns = 4;
  int m_MinimumCardWidth = 220;
  int m_AppliedColumns = 0;
};

void StatisticsViewer::BuildOverview()
{
  QWidget *old = m_Overview->takeWidget();
  delete old;
  QWidget *page = new QWidget;
  page->setObjectName(lit("statisticsOverview"));
  // Keep the application's UE palette and native button states. Only dashboard
  // geometry and typography are customised here, not a separate colour theme.
  page->setStyleSheet(OverviewStyleSheet());
  QVBoxLayout *layout = new QVBoxLayout(page);
  layout->setSizeConstraint(QLayout::SetNoConstraint);
  page->setMinimumSize(0, 0);
  layout->setContentsMargins(20, 20, 20, 20);
  layout->setSpacing(16);
  auto label = [](const QString &text, const char *role) { return OverviewLabel(text, role); };
  auto bytes = [](uint64_t value) {
    return QFormatStr("%1 MiB").arg(double(value) / (1024.0 * 1024.0), 0, 'f', 2);
  };
  auto stripedRow = [](int index) {
    QFrame *frame = new QFrame;
    frame->setProperty("stripe", index % 2 ? lit("odd") : lit("even"));
    frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    return frame;
  };
  QVBoxLayout *sections = nullptr;
  auto section = [&](const QString &title, const QString &subtitle) {
    QFrame *frame = new QFrame;
    frame->setProperty("card", true);
    QVBoxLayout *box = new QVBoxLayout(frame);
    frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    box->setContentsMargins(20, 18, 20, 18);
    box->setSpacing(12);
    box->addWidget(label(title, "heading"));
    if(!subtitle.isEmpty()) box->addWidget(label(subtitle, "caption"));
    if(sections)
      sections->addWidget(frame);
    else
      layout->addWidget(frame);
    return box;
  };
  const FrameDescription &info = m_Ctx.FrameInfo();
  QVBoxLayout *hero = section(tr("FRAME OVERVIEW"), QString());
  hero->addWidget(label(QFileInfo(m_Ctx.GetCaptureFilename()).fileName(), "title"));
  hero->addWidget(label(tr("Capture activity, resource composition and recorded API statistics."), "caption"));
  hero->addWidget(label(tr("%1   /   CAPTURE DATA   /   NO HEURISTIC METRICS")
      .arg(ToQStr(m_Ctx.APIProps().pipelineType)), "badge"));
  uint32_t draws = 0, dispatches = 0, markers = 0;
  uint64_t events = 0;
  uint32_t maxEid = 0;
  std::function<void(const ActionDescription &)> count = [&](const ActionDescription &a) {
    events++;
    maxEid = qMax(maxEid, a.eventId);
    for(const ActionDescription &child : a.children) count(child);
  };
  for(const ActionDescription &a : m_Ctx.CurRootActions())
  {
    CountContributingEvents(a, draws, dispatches, markers);
    count(a);
  }
  uint64_t textureBytes = 0, rtBytes = 0, bufferBytes = 0, ibBytes = 0, vbBytes = 0;
  uint64_t largeBytes = 0, widthSum = 0, heightSum = 0;
  int ordinaryTextures = 0, rtCount = 0;
  struct ResourceRow { QString name; uint64_t size; QString detail; ResourceId id; bool texture; };
  std::vector<ResourceRow> resources;
  for(const TextureDescription &t : m_Ctx.GetTextures())
  {
    const bool target = bool(t.creationFlags & (TextureCategory::ColorTarget | TextureCategory::DepthTarget));
    if(target) { rtBytes += t.byteSize; rtCount++; }
    else
    {
      textureBytes += t.byteSize;
      ordinaryTextures++;
      widthSum += t.width; heightSum += t.height;
      if(t.width > 32 && t.height > 32) largeBytes += t.byteSize;
    }
    resources.push_back({m_Ctx.GetResourceName(t.resourceId), t.byteSize,
        tr("%1 × %2 × %3 | %4 | %5").arg(t.width).arg(t.height).arg(t.depth)
          .arg(QString(t.format.Name())).arg(target ? tr("Render/depth target") : tr("Texture")), t.resourceId, true});
  }
  for(const BufferDescription &b : m_Ctx.GetBuffers())
  {
    bufferBytes += b.length;
    if(b.creationFlags & BufferCategory::Index) ibBytes += b.length;
    if(b.creationFlags & BufferCategory::Vertex) vbBytes += b.length;
    resources.push_back({m_Ctx.GetResourceName(b.resourceId), b.length, tr("Buffer"), b.resourceId, false});
  }
  const uint64_t total = textureBytes + rtBytes + bufferBytes;
  QGridLayout *metrics = new QGridLayout;
  metrics->setSpacing(12);
  for(int column = 0; column < 3; column++) metrics->setColumnStretch(column, 1);
  layout->addLayout(metrics);
  int metricIndex = 0;
  auto metric = [&](const QString &title, const QString &value, const QString &caption) {
    QFrame *frame = new QFrame;
    frame->setProperty("card", true);
    frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QVBoxLayout *box = new QVBoxLayout(frame);
    box->setContentsMargins(16, 12, 16, 12);
    box->addWidget(label(title.toUpper(), "metricTitle"));
    QLabel *valueLabel = label(value, "value");
    box->addWidget(valueLabel);
    box->addWidget(label(caption, "caption"));
    metrics->addWidget(frame, metricIndex / 3, metricIndex % 3);
    metricIndex++;
    return valueLabel;
  };
  metric(tr("Draw calls"), QString::number(draws), tr("Includes mesh dispatch actions"));
  metric(tr("Dispatch calls"), QString::number(dispatches), tr("Compute dispatch actions"));
  metric(tr("Action nodes"), QString::number(events), tr("Includes markers; not an API call count"));
  metric(tr("Resource size"), bytes(total), tr("Textures + buffers; not measured GPU residency"));
  metric(tr("Render targets"), bytes(rtBytes), tr("%1 color/depth resources").arg(rtCount));
  QLabel *gpuValue = metric(tr("GPU event durations"), tr("Not collected"), tr("Sum of measured events; not frame wall-clock time"));
  QPushButton *measure = new QPushButton(tr("Measure GPU durations"));
  measure->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QVBoxLayout *gpuCard = qobject_cast<QVBoxLayout *>(gpuValue->parentWidget()->layout());
  gpuCard->addWidget(measure, 0, Qt::AlignLeft);
  connect(measure, &QPushButton::clicked, this, [this, measure, gpuValue]() {
    measure->setEnabled(false);
    gpuValue->setText(tr("Measuring…"));
    const uint64_t generation = m_Generation;
    QPointer<StatisticsViewer> viewer(this);
    QPointer<QLabel> value(gpuValue);
    QPointer<QPushButton> button(measure);
    m_Ctx.Replay().AsyncInvoke([viewer, value, button, generation](IReplayController *controller) {
      const auto available = controller->EnumerateCounters();
      bool supported = false;
      for(GPUCounter counter : available) if(counter == GPUCounter::EventGPUDuration) supported = true;
      double seconds = 0.0;
      uint32_t samples = 0;
      if(supported)
      {
        rdcarray<GPUCounter> counters;
        counters.push_back(GPUCounter::EventGPUDuration);
        const auto results = controller->FetchCounters(counters);
        for(const CounterResult &r : results)
          if(r.counter == GPUCounter::EventGPUDuration && std::isfinite(r.value.d) && r.value.d >= 0.0)
          { seconds += r.value.d; samples++; }
      }
      if(!viewer) return;
      GUIInvoke::call(viewer, [viewer, value, button, generation, seconds, samples]() {
        if(!viewer || viewer->m_Generation != generation || !value || !button) return;
        value->setText(samples ? viewer->tr("%1 ms (%2 events)").arg(seconds * 1000.0, 0, 'f', 3).arg(samples)
                               : viewer->tr("Unavailable"));
        button->setEnabled(true);
      });
    });
  });
  sections = new QVBoxLayout;
  sections->setSpacing(16);
  layout->addLayout(sections);
  QVBoxLayout *memory = section(tr("Resource composition"), tr("Resource description sizes, not traffic or physical allocation. RTs are excluded from the texture bucket."));
  auto bar = [&](const QString &name, uint64_t size) {
    double percent = total ? 100.0 * double(size) / double(total) : 0.0;
    memory->addWidget(label(tr("%1    %2    %3%").arg(name).arg(bytes(size)).arg(percent, 0, 'f', 1), "caption"));
    QProgressBar *progress = new QProgressBar;
    progress->setRange(0, 1000);
    progress->setValue(int(percent * 10.0));
    progress->setTextVisible(false);
    memory->addWidget(progress);
  };
  bar(tr("Textures (non-RT)"), textureBytes);
  bar(tr("Render/depth targets"), rtBytes);
  bar(tr("Buffers"), bufferBytes);
  memory->addWidget(label(tr("%1 textures total · %2 buffers\nTextures >32 × 32 (non-RT): %3 · Index buffers: %4 · Vertex buffers: %5")
      .arg(m_Ctx.GetTextures().count()).arg(m_Ctx.GetBuffers().count()).arg(bytes(largeBytes)).arg(bytes(ibBytes)).arg(bytes(vbBytes)), "caption"));
  if(ordinaryTextures)
    memory->addWidget(label(tr("Average non-RT texture: %1 × %2")
        .arg(double(widthSum) / ordinaryTextures, 0, 'f', 1).arg(double(heightSum) / ordinaryTextures, 0, 'f', 1), "caption"));
  QVBoxLayout *calls = section(tr("API activity"), tr("Recorded API invocation counts, not unique state changes. Per-stage details and distributions are retained in Detailed report."));
  if(!info.stats.recorded)
    calls->addWidget(label(tr("Not collected for this capture. Missing counters are not zero."), "caption"));
  else
  {
    uint64_t shaders = 0, constants = 0, samplers = 0, bindings = 0;
    for(auto s : indices<ShaderStage>())
    {
      shaders += info.stats.shaders[s].calls; constants += info.stats.constants[s].calls;
      samplers += info.stats.samplers[s].calls; bindings += info.stats.resources[s].calls;
    }
    calls->addWidget(label(tr("Shader sets: %1 · Constant binds: %2 · Sampler binds: %3 · Resource binds: %4")
        .arg(shaders).arg(constants).arg(samplers).arg(bindings), "caption"));
    calls->addWidget(label(tr("Output sets: %1 · Resource updates: %2 · Blend sets: %3 · Depth/stencil sets: %4 · Rasterizer sets: %5")
        .arg(info.stats.outputs.calls).arg(info.stats.updates.calls).arg(info.stats.blends.calls)
        .arg(info.stats.depths.calls).arg(info.stats.rasters.calls), "caption"));
  }
  QVBoxLayout *passBox = section(tr("Marker group activity"),
      tr("Recorded top-level action groups, not inferred passes. Counts are workload volume, not GPU cost."));
  int groups = 0;
  for(const ActionDescription &a : m_Ctx.CurRootActions())
  {
    if(a.children.empty()) continue;
    uint32_t groupDraws = 0, groupDispatches = 0, groupMarkers = 0;
    CountContributingEvents(a, groupDraws, groupDispatches, groupMarkers);
    if(!groupDraws && !groupDispatches) continue;
    const uint32_t eid = a.eventId;
    QFrame *rowFrame = stripedRow(groups);
    QHBoxLayout *row = new QHBoxLayout(rowFrame);
    row->setContentsMargins(12, 7, 12, 7);
    row->setSpacing(12);
    QVBoxLayout *description = new QVBoxLayout;
    description->setSpacing(3);
    description->addWidget(label(QString(a.GetName(m_Ctx.GetStructuredFile())), "heading"));
    description->addWidget(label(tr("EID %1 · %2 draws · %3 dispatches")
        .arg(eid).arg(groupDraws).arg(groupDispatches), "caption"));
    row->addLayout(description, 1);
    QPushButton *jump = new QPushButton(tr("Go to event"));
    jump->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(jump, &QPushButton::clicked, this, [this, eid]() { m_Ctx.SetEventID({}, eid, eid); });
    row->addWidget(jump, 0, Qt::AlignVCenter);
    passBox->addWidget(rowFrame);
    groups++;
  }
  if(!groups) passBox->addWidget(label(tr("No top-level action groups with draws or dispatches."), "caption"));

  const SystemTraceSummary &traceSummary = CurrentSystemTraceSummary();
  if(traceSummary.loaded)
  {
    QVBoxLayout *trace = section(tr("System trace"),
        tr("Measured by the trace importer and reported as recorded. Absent measurements are shown "
           "as unavailable, not as zero."));
    trace->addWidget(label(tr("%1  ·  %2  ·  %3  ·  %4")
        .arg(QFileInfo(traceSummary.path).fileName(), traceSummary.formatName,
             traceSummary.timeUnit.isEmpty() ? tr("unit not declared")
                                             : tr("timestamps in %1").arg(traceSummary.timeUnit),
             traceSummary.declaredFrameNumbers ? tr("frame numbers declared by the source")
                                               : tr("frame boundaries derived by the importer")),
        "caption"));
    trace->addWidget(label(tr("Span %1 s  ·  %2 counter lanes  ·  %3 thread lanes  "
                              "·  %4 spans  ·  %5 frame boundaries")
        .arg(traceSummary.spanSec, 0, 'f', 3).arg(traceSummary.counterLanes)
        .arg(traceSummary.threadLanes).arg(traceSummary.spanCount)
        .arg(traceSummary.frameBoundaries), "caption"));
    const TraceGpuStats &gpu = traceSummary.gpuStats;
    trace->addWidget(label(gpu.commandBatches
        ? tr("GPU command stream: %1 batches  ·  %2 with validated execution windows  "
             "·  %3 without counters  ·  %4 rejected")
              .arg(gpu.commandBatches).arg(gpu.anchoredBatches)
              .arg(gpu.batchesWithoutCounters).arg(gpu.rejectedBatches)
        : tr("GPU command stream: no kgsl command batches were recorded in this trace."),
        "caption"));
    if(!traceSummary.warnings.isEmpty())
      trace->addWidget(label(tr("%1 importer warning(s) on the Warnings tab of the System Trace "
                                "viewer. They describe what was missing or dropped.")
          .arg(traceSummary.warnings.size()), "caption"));

    if(!traceSummary.topEvents.isEmpty())
    {
      trace->addWidget(label(tr("Most frequent recorded events"), "metricTitle"));
      for(int i = 0; i < traceSummary.topEvents.size(); i++)
      {
        QFrame *rowFrame = stripedRow(i);
        QHBoxLayout *row = new QHBoxLayout(rowFrame);
        row->setContentsMargins(12, 5, 12, 5);
        row->addWidget(label(traceSummary.topEvents[i].first, "caption"), 1);
        row->addWidget(label(QString::number(traceSummary.topEvents[i].second), "caption"), 0,
                       Qt::AlignRight);
        trace->addWidget(rowFrame);
      }
    }

    if(!traceSummary.topThreads.isEmpty())
    {
      trace->addWidget(label(tr("Busiest threads by measured running time"), "metricTitle"));
      for(int i = 0; i < traceSummary.topThreads.size(); i++)
      {
        const TraceThreadTime &thread = traceSummary.topThreads[i];
        QFrame *rowFrame = stripedRow(i);
        QHBoxLayout *row = new QHBoxLayout(rowFrame);
        row->setContentsMargins(12, 5, 12, 5);
        row->addWidget(label(thread.process.isEmpty()
                                 ? tr("%1 (tid %2)").arg(thread.name).arg(thread.tid)
                                 : tr("%1  ·  %2 (tid %3)").arg(thread.name, thread.process).arg(thread.tid),
                             "caption"),
                       1);
        row->addWidget(label(tr("%1 ms running  ·  %2 switches")
                                 .arg(thread.runningSec * 1000.0, 0, 'f', 1).arg(thread.switches),
                             "caption"),
                       0, Qt::AlignRight);
        trace->addWidget(rowFrame);
      }
    }
  }
  QVBoxLayout *hotspots = section(tr("Largest resources"), tr("Top 8 by declared size. Size ranking does not imply GPU cost or a performance bottleneck."));
  std::sort(resources.begin(), resources.end(), [](const ResourceRow &a, const ResourceRow &b) { return a.size > b.size; });
  for(size_t i = 0; i < qMin(size_t(8), resources.size()); i++)
  {
    const ResourceRow &r = resources[i];
    QFrame *rowFrame = stripedRow(int(i));
    QHBoxLayout *row = new QHBoxLayout(rowFrame);
    row->setContentsMargins(12, 7, 12, 7);
    row->setSpacing(12);
    QVBoxLayout *description = new QVBoxLayout;
    description->setSpacing(3);
    description->addWidget(label(r.name, "heading"));
    description->addWidget(label(r.detail, "caption"));
    row->addLayout(description, 1);
    QLabel *size = label(bytes(r.size), "caption");
    size->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    row->addWidget(size, 0, Qt::AlignVCenter);
    if(r.texture)
    {
      QPushButton *open = new QPushButton(tr("Inspect texture"));
      const ResourceId id = r.id;
      connect(open, &QPushButton::clicked, this, [this, id]() {
        m_Ctx.ShowTextureViewer();
        m_Ctx.GetTextureViewer()->ViewTexture(id, CompType::Typeless, true);
      });
      open->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      row->addWidget(open, 0, Qt::AlignVCenter);
    }
    hotspots->addWidget(rowFrame);
  }
  if(resources.empty()) hotspots->addWidget(label(tr("No texture or buffer resources."), "caption"));
  QVBoxLayout *storage = section(tr("Capture context"), m_Ctx.GetCaptureFilename());
  storage->addWidget(label(tr("Compressed: %1 · Uncompressed: %2\nPersistent data (approx.): %3 · Initial data (approx.): %4")
      .arg(bytes(info.compressedFileSize)).arg(bytes(info.uncompressedFileSize))
      .arg(bytes(info.persistentSize)).arg(bytes(info.initDataSize)), "caption"));
  storage->addWidget(label(tr("API-call estimate: %1 (max EID minus draw, dispatch and marker nodes; not an exact event enumeration)")
      .arg(uint64_t(maxEid) > uint64_t(draws) + dispatches + markers ? uint64_t(maxEid) - draws - dispatches - markers : 0), "caption"));
  layout->addStretch();
  m_Overview->setWidget(page);
  m_Overview->verticalScrollBar()->setValue(0);
}

// The trace report is the measurement-by-measurement view of the trace that the overview summarises:
// one card per published value, three across while there is room and fewer as the panel narrows. It
// reads the same published summary, so it states exactly what the importer recorded and nothing that
// would have to be inferred from it.
void StatisticsViewer::BuildTraceReport()
{
  QWidget *old = m_TraceReport->takeWidget();
  delete old;

  QWidget *page = new QWidget;
  page->setObjectName(lit("statisticsTraceReport"));
  page->setStyleSheet(OverviewStyleSheet());
  QVBoxLayout *layout = new QVBoxLayout(page);
  layout->setSizeConstraint(QLayout::SetNoConstraint);
  page->setMinimumSize(0, 0);
  layout->setContentsMargins(20, 20, 20, 20);
  layout->setSpacing(16);

  auto label = [](const QString &text, const char *role) { return OverviewLabel(text, role); };
  auto stripedRow = [](int index) {
    QFrame *frame = new QFrame;
    frame->setProperty("stripe", index % 2 ? lit("odd") : lit("even"));
    frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    return frame;
  };
  auto section = [&](const QString &title, const QString &subtitle) {
    QFrame *frame = OverviewCard();
    QVBoxLayout *box = new QVBoxLayout(frame);
    box->setContentsMargins(20, 18, 20, 18);
    box->setSpacing(12);
    box->addWidget(label(title, "heading"));
    if(!subtitle.isEmpty()) box->addWidget(label(subtitle, "caption"));
    layout->addWidget(frame);
    return box;
  };

  const SystemTraceSummary &trace = CurrentSystemTraceSummary();

  QVBoxLayout *hero = section(tr("TRACE DOCUMENT"), QString());
  if(!trace.loaded)
  {
    hero->addWidget(label(tr("No trace loaded"), "title"));
    hero->addWidget(label(tr("Import a trace file, open an RDT archive, or record a trace from the "
                             "connected device in the System Trace panel. This report fills in as soon "
                             "as a trace is published."),
                         "caption"));
    hero->addWidget(label(tr("NO TRACE   /   NOT COLLECTED"), "badge"));
    layout->addStretch();
    m_TraceReport->setWidget(page);
    m_TraceReport->verticalScrollBar()->setValue(0);
    return;
  }

  const QString fileName = QFileInfo(trace.path).fileName();
  hero->addWidget(label(fileName.isEmpty() ? tr("Imported trace") : fileName, "title"));
  hero->addWidget(label(trace.path, "caption"));
  hero->addWidget(label(tr("%1   /   %2   /   TRACE MEASUREMENTS")
      .arg(trace.formatName, trace.timeUnit.isEmpty() ? tr("unit not declared") : trace.timeUnit),
      "badge"));
  hero->addWidget(
      label(tr("Span %1 s  \u00b7  %2 spans  \u00b7  %3 frame boundaries  \u00b7  %4 counter lanes  "
               "\u00b7  %5 thread lanes")
                .arg(trace.spanSec, 0, 'f', 3)
                .arg(trace.spanCount)
                .arg(trace.frameBoundaries)
                .arg(trace.counterLanes)
                .arg(trace.threadLanes),
            "caption"));

  // Cards are added in reading order; the grid decides how many fit per row. Eighteen measurements at
  // three per row fill six rows exactly at full width.
  CardGrid *grid = new CardGrid(3, 260);
  layout->addWidget(grid);

  auto metric = [&](const QString &title, const QString &value, const QString &unit,
                    const QString &detail, const QString &tag) {
    QFrame *card = OverviewCard();
    QVBoxLayout *box = new QVBoxLayout(card);
    box->setContentsMargins(16, 14, 16, 14);
    box->setSpacing(8);
    QHBoxLayout *header = new QHBoxLayout;
    header->setSpacing(8);
    header->addWidget(label(title.toUpper(), "metricTitle"), 1, Qt::AlignTop);
    QLabel *pill = label(tag, "pill");
    pill->setWordWrap(false);
    pill->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    header->addWidget(pill, 0, Qt::AlignTop);
    box->addLayout(header);
    QHBoxLayout *valueRow = new QHBoxLayout;
    valueRow->setSpacing(6);
    valueRow->addWidget(label(value, "metric"), 0, Qt::AlignBottom);
    if(!unit.isEmpty()) valueRow->addWidget(label(unit, "caption"), 0, Qt::AlignBottom);
    valueRow->addStretch(1);
    box->addLayout(valueRow);
    if(!detail.isEmpty()) box->addWidget(label(detail, "caption"));
    box->addStretch(1);
    grid->addCard(card);
  };

  const TraceGpuStats &gpu = trace.gpuStats;

  metric(tr("Trace span"), QString::number(trace.spanSec, 'f', 3), tr("s"),
         tr("Extent of the trace's own timeline."), tr("TIME"));
  metric(tr("Timestamp unit"), trace.timeUnit.isEmpty() ? tr("Not declared") : trace.timeUnit,
         QString(), tr("Unit of the timestamps the source recorded."), tr("TIME"));
  metric(tr("Spans"), QString::number(trace.spanCount), QString(),
         tr("Thread-state spans the importer kept."), tr("CPU"));
  metric(tr("Thread lanes"), QString::number(trace.threadLanes), QString(),
         tr("Per-thread lanes carrying measured running time."), tr("CPU"));
  metric(tr("Counter lanes"), QString::number(trace.counterLanes), QString(),
         tr("Sampled counter series drawn as lanes."), tr("COUNTERS"));
  metric(tr("Frame boundaries"), QString::number(trace.frameBoundaries), QString(),
         tr("Frame edges found in the trace."), tr("FRAMES"));
  metric(tr("Frame numbering"), trace.declaredFrameNumbers ? tr("Declared") : tr("Derived"), QString(),
         trace.declaredFrameNumbers
             ? tr("The source numbered its own frames.")
             : tr("Boundaries are ordinal: the source declared no frame numbers."),
         tr("FRAMES"));
  metric(tr("Importer warnings"), QString::number(trace.warnings.size()), QString(),
         tr("Missing or dropped records, listed below."), tr("IMPORT"));
  metric(tr("GPU batches"), QString::number(gpu.commandBatches), QString(),
         gpu.commandBatches
             ? tr("kgsl batches whose submission and retirement were both in the trace.")
             : tr("No kgsl command batches were recorded."),
         tr("GPU"));
  metric(tr("Validated windows"), QString::number(gpu.anchoredBatches), QString(),
         tr("Batches with an execution window checked against submit to retire."), tr("GPU"));
  metric(tr("Without counters"), QString::number(gpu.batchesWithoutCounters), QString(),
         tr("Batches with no counter readings, so only queued to retired is known."), tr("GPU"));
  metric(tr("Rejected batches"), QString::number(gpu.rejectedBatches), QString(),
         tr("Batches whose counters did not bracket their own interval."), tr("GPU"));
  metric(tr("Open at end"), QString::number(gpu.unterminatedBatches), QString(),
         tr("%1 fence wait(s) were still open when the recording stopped.")
             .arg(gpu.unterminatedWaits),
         tr("GPU"));
  metric(tr("GPU power states"), QString::number(gpu.powerStateChanges), QString(),
         tr("Power-state changes the driver published."), tr("GPU"));
  metric(tr("Thermal zones"), QString::number(gpu.thermalZones), QString(),
         tr("%1 trip point(s) crossed by a GPU zone.").arg(gpu.thermalTrips), tr("GPU"));
  metric(tr("Live GPU samples"), QString::number(gpu.liveGpuSamples), QString(),
         tr("%1 of them bracketed an advancing elapsed counter.").arg(gpu.liveGpuWindowSamples),
         tr("GPU"));
  if(!trace.topEvents.isEmpty())
    metric(tr("Most frequent event"), QString::number(trace.topEvents.first().second), QString(),
           trace.topEvents.first().first, tr("EVENTS"));
  if(!trace.topThreads.isEmpty())
  {
    const TraceThreadTime &thread = trace.topThreads.first();
    metric(tr("Busiest thread"), QString::number(thread.runningSec * 1000.0, 'f', 1),
           tr("ms running"),
           thread.process.isEmpty()
               ? tr("%1 (tid %2)").arg(thread.name).arg(thread.tid)
               : tr("%1  \u00b7  %2 (tid %3)").arg(thread.name, thread.process).arg(thread.tid),
           tr("CPU"));
  }

  QVBoxLayout *events = section(tr("Most frequent recorded events"),
      tr("How often each event type appears in the trace. A count is not a duration."));
  if(trace.topEvents.isEmpty())
    events->addWidget(label(tr("No recognised events in this trace."), "caption"));
  for(int i = 0; i < trace.topEvents.size(); i++)
  {
    QFrame *rowFrame = stripedRow(i);
    QHBoxLayout *row = new QHBoxLayout(rowFrame);
    row->setContentsMargins(12, 6, 12, 6);
    row->addWidget(label(trace.topEvents[i].first, "caption"), 1);
    row->addWidget(label(QString::number(trace.topEvents[i].second), "caption"), 0, Qt::AlignRight);
    events->addWidget(rowFrame);
  }

  QVBoxLayout *threads = section(tr("Busiest threads"),
      tr("Measured running time over the whole trace, per lane. The bar is a share of the busiest "
         "lane in this list, not of the whole trace. Kernel idle lanes (tid 0) are excluded because "
         "their running time is CPU idleness, not work."));
  if(trace.topThreads.isEmpty())
    threads->addWidget(label(tr("No thread lane carried running time."), "caption"));
  const double busiest = trace.topThreads.isEmpty() ? 0.0 : trace.topThreads.first().runningSec;
  for(int i = 0; i < trace.topThreads.size(); i++)
  {
    const TraceThreadTime &thread = trace.topThreads[i];
    QFrame *rowFrame = stripedRow(i);
    QHBoxLayout *row = new QHBoxLayout(rowFrame);
    row->setContentsMargins(12, 7, 12, 7);
    row->setSpacing(12);
    row->addWidget(label(thread.process.isEmpty()
                             ? tr("%1 (tid %2)").arg(thread.name).arg(thread.tid)
                             : tr("%1  \u00b7  %2 (tid %3)").arg(thread.name, thread.process).arg(thread.tid),
                         "caption"),
                   1);
    if(busiest > 0.0)
    {
      QProgressBar *bar = new QProgressBar;
      bar->setRange(0, 1000);
      bar->setValue(int(thread.runningSec / busiest * 1000.0));
      bar->setTextVisible(false);
      bar->setFixedWidth(150);
      bar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      row->addWidget(bar, 0, Qt::AlignVCenter);
    }
    row->addWidget(label(tr("%1 ms running  \u00b7  %2 switches")
                             .arg(thread.runningSec * 1000.0, 0, 'f', 1)
                             .arg(thread.switches),
                         "caption"),
                   0, Qt::AlignRight);
    threads->addWidget(rowFrame);
  }

  QVBoxLayout *warnings = section(tr("Importer warnings"),
      tr("What the import found missing, ambiguous or dropped. This is the importer's own list, not a "
         "diagnosis of the trace's subject."));
  if(trace.warnings.isEmpty())
    warnings->addWidget(label(tr("No warnings: every record the importer read was accepted."), "caption"));
  const int shownWarnings = qMin(trace.warnings.size(), 40);
  for(int i = 0; i < shownWarnings; i++)
  {
    QFrame *rowFrame = stripedRow(i);
    QHBoxLayout *row = new QHBoxLayout(rowFrame);
    row->setContentsMargins(12, 6, 12, 6);
    row->addWidget(label(trace.warnings[i], "caption"), 1);
    warnings->addWidget(rowFrame);
  }
  if(trace.warnings.size() > shownWarnings)
    warnings->addWidget(label(tr("%1 more warning(s) are on the Warnings tab of the System Trace viewer.")
                                 .arg(trace.warnings.size() - shownWarnings),
                             "caption"));

  layout->addStretch();
  m_TraceReport->setWidget(page);
  m_TraceReport->verticalScrollBar()->setValue(0);
}

StatisticsViewer::StatisticsViewer(ICaptureContext &ctx, QWidget *parent)
    : QFrame(parent), ui(new Ui::StatisticsViewer), m_Ctx(ctx)
{
  ui->setupUi(this);

  ui->statistics->setFont(Formatter::FixedFont());
  ui->horizontalLayout->removeWidget(ui->statistics);
  QTabWidget *tabs = new QTabWidget(this);
  m_Overview = new QScrollArea(tabs);
  m_Overview->setWidgetResizable(true);
  m_Overview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_Overview->setFrameShape(QFrame::NoFrame);
  tabs->addTab(m_Overview, tr("Overview"));
  m_TraceReport = new QScrollArea(tabs);
  m_TraceReport->setWidgetResizable(true);
  m_TraceReport->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_TraceReport->setFrameShape(QFrame::NoFrame);
  tabs->addTab(m_TraceReport, tr("Trace Report"));
  tabs->addTab(ui->statistics, tr("Detailed report"));
  ui->horizontalLayout->addWidget(tabs);

  // A trace can arrive after this window is open (import from a file, an RDT archive, or a remote
  // launch), so the overview follows the published trace summary instead of only reading it when a
  // capture loads.
  QPointer<StatisticsViewer> guard(this);
  SetSystemTraceSummaryCallback([guard]() {
    // A trace can be imported with no capture open, in which case there is nothing for this window
    // to add to: the overview is built from the capture's actions and resources.
    if(!guard || !guard->m_Ctx.IsCaptureLoaded())
      return;
    guard->BuildOverview();
    guard->BuildTraceReport();
  });

  m_Ctx.AddCaptureViewer(this);
}

StatisticsViewer::~StatisticsViewer()
{
  SetSystemTraceSummaryCallback(std::function<void()>());

  m_Ctx.BuiltinWindowClosed(this);

  m_Ctx.RemoveCaptureViewer(this);
  delete ui;
}

void StatisticsViewer::OnCaptureClosed()
{
  ++m_Generation;
  delete m_Overview->takeWidget();
  delete m_TraceReport->takeWidget();
  ui->statistics->clear();
}

void StatisticsViewer::OnCaptureLoaded()
{
  ++m_Generation;
  BuildOverview();
  BuildTraceReport();
  GenerateReport();
  ui->statistics->setText(m_Report);
}

#include "RenderGraph.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace RenderGraph
{
AccessKind Classify(ResourceUsage u)
{
  if(u == ResourceUsage::VertexBuffer || u == ResourceUsage::IndexBuffer ||
     (u >= ResourceUsage::VS_Constants && u <= ResourceUsage::All_Constants) ||
     (u >= ResourceUsage::VS_Resource && u <= ResourceUsage::All_Resource) ||
     u == ResourceUsage::InputTarget || u == ResourceUsage::Indirect ||
     u == ResourceUsage::CopySrc || u == ResourceUsage::ResolveSrc)
    return AccessKind::Read;
  if((u >= ResourceUsage::VS_RWResource && u <= ResourceUsage::All_RWResource) ||
     u == ResourceUsage::ColorTarget || u == ResourceUsage::DepthStencilTarget ||
     u == ResourceUsage::GenMips || u == ResourceUsage::Resolve || u == ResourceUsage::Copy)
    return AccessKind::ReadWrite;
  if(u == ResourceUsage::StreamOut || u == ResourceUsage::Clear ||
     u == ResourceUsage::CopyDst || u == ResourceUsage::ResolveDst || u == ResourceUsage::CPUWrite)
    return AccessKind::Write;
  if(u == ResourceUsage::Discard) return AccessKind::Discard;
  if(u == ResourceUsage::Barrier || u == ResourceUsage::Unused) return AccessKind::Ordering;
  return AccessKind::Unknown;
}
QString AccessName(AccessKind k)
{
  switch(k)
  {
    case AccessKind::Read: return QStringLiteral("Read");
    case AccessKind::Write: return QStringLiteral("Write");
    case AccessKind::ReadWrite: return QStringLiteral("Potential read / write");
    case AccessKind::Discard: return QStringLiteral("Discard");
    case AccessKind::Ordering: return QStringLiteral("Ordering only");
    default: return QStringLiteral("Unknown access");
  }
}
bool Build(Snapshot &g, const std::atomic_bool &cancelled)
{
  g.accesses.clear(); g.links.clear();
  g.externalReads = g.unassigned = g.unsupported = 0;
  for(Pass &p : g.passes)
  {
    p.inputs = p.outputs = p.layer = p.row = 0;
    p.inputNames.clear(); p.outputNames.clear();
  }
  std::map<std::pair<int, int>, int> edges;
  QVector<std::set<int>> inputs(g.passes.size()), outputs(g.passes.size());
  for(int ri = 0; ri < g.resources.size(); ++ri)
  {
    if(cancelled.load()) return false;
    Resource &r = g.resources[ri];
    std::sort(r.usages.begin(), r.usages.end());
    int writer = -1, version = 0;
    uint32_t writeEvent = 0;
    for(const EventUsage &u : r.usages)
    {
      const AccessKind kind = Classify(u.usage);
      if(kind == AccessKind::Ordering) continue;
      auto found = g.eventPass.find(u.eventId);
      if(found == g.eventPass.end())
      {
        // An unassigned write invalidates the previous producer: never invent a dependency.
        if(kind != AccessKind::Read) { writer = -1; ++version; }
        ++g.unassigned;
        continue;
      }
      const int pi = found->second;
      const bool read = kind == AccessKind::Read || kind == AccessKind::ReadWrite;
      const bool write = kind == AccessKind::Write || kind == AccessKind::ReadWrite;
      Access a; a.resource = ri; a.pass = pi; a.event = u.eventId;
      a.usage = u.usage; a.kind = kind; a.version = version; a.producer = writer;
      const int ai = g.accesses.size();
      if(read)
      {
        inputs[pi].insert(ri);
        if(writer >= 0 && writer != pi)
        {
          const auto key = std::make_pair(writer, pi);
          auto edge = edges.find(key);
          int li;
          if(edge == edges.end())
          {
            li = g.links.size(); edges[key] = li;
            Link link; link.source = writer; link.destination = pi; g.links.push_back(link);
          }
          else li = edge->second;
          g.links[li].evidence.push_back(ai);
        }
        else if(writer < 0) ++g.externalReads;
      }
      if(write)
      {
        outputs[pi].insert(ri);
        if(writer != pi || writeEvent != u.eventId) ++version;
        writer = pi; writeEvent = u.eventId;
        // Read-write evidence refers to its incoming version; pure writes to the new version.
        a.outputVersion = version;
        if(!read) a.version = version;
      }
      if(kind == AccessKind::Discard) { writer = -1; ++version; }
      if(kind == AccessKind::Unknown) { ++g.unsupported; writer = -1; ++version; }
      g.accesses.push_back(a);
    }
  }
  // Passes are non-overlapping chronological intervals, so dependencies must point forwards.
  QVector<int> occupied;
  QVector<QVector<int>> incoming(g.passes.size());
  for(const Link &l : g.links)
    if(l.source < l.destination) incoming[l.destination].push_back(l.source);
  for(int pi = 0; pi < g.passes.size(); ++pi)
  {
    if(cancelled.load()) return false;
    Pass &p = g.passes[pi];
    p.inputs = int(inputs[pi].size()); p.outputs = int(outputs[pi].size());
    for(int ri : inputs[pi]) p.inputNames.push_back(g.resources[ri].name);
    for(int ri : outputs[pi]) p.outputNames.push_back(g.resources[ri].name);
    std::sort(p.inputNames.begin(),p.inputNames.end());
    std::sort(p.outputNames.begin(),p.outputNames.end());
    // Use the shallowest valid dependency layer. The old monotonic column cursor made long
    // captures thousands of pixels wide even when passes were independent.
    p.layer = 0;
    for(int source : incoming[pi]) p.layer = qMax(p.layer, g.passes[source].layer + 1);
    while(occupied.size() <= p.layer) occupied.push_back(0);
    // Wrap crowded layers into visual continuation columns. This affects placement only and does
    // not manufacture a dependency between neighbouring columns.
    while(occupied[p.layer] >= 7)
    {
      ++p.layer;
      while(occupied.size() <= p.layer) occupied.push_back(0);
    }
    p.row = occupied[p.layer]++;
  }
  g.layers = occupied.size(); g.rows = 0;
  for(int n : occupied) g.rows = qMax(g.rows, n);
  return !cancelled.load();
}
QVector<TimingSummary> SummariseTimings(const Snapshot &g,
                                       const std::map<uint32_t, double> &seconds)
{
  QVector<TimingSummary> result(g.passes.size());
  for(int pi = 0; pi < g.passes.size(); ++pi)
  {
    std::set<uint32_t> events(g.passes[pi].events.begin(), g.passes[pi].events.end());
    result[pi].expected = int(events.size());
    for(uint32_t event : events)
    {
      auto found = seconds.find(event);
      if(found != seconds.end() && std::isfinite(found->second) && found->second >= 0.0)
      {
        result[pi].seconds += found->second;
        ++result[pi].samples;
      }
    }
  }
  return result;
}
}

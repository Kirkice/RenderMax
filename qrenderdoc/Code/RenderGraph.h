#pragma once

#include <QString>
#include <QVector>
#include <atomic>
#include <map>
#include "renderdoc_replay.h"

namespace RenderGraph
{
enum class AccessKind { Read, Write, ReadWrite, Discard, Ordering, Unknown };
struct Command
{
  uint32_t event = 0;
  QString name, kind;
};
struct Pass
{
  QString name, path, kind;
  // hierarchy contains display ancestors from outermost to immediate parent.
  QStringList hierarchy, inputNames, outputNames;
  uint32_t first = 0, last = 0, primaryEvent = 0;
  int draws = 0, dispatches = 0, transfers = 0, clears = 0;
  int inputs = 0, outputs = 0, layer = 0, row = 0;
  bool inferred = false;
  QVector<uint32_t> events;
  QVector<Command> commands;
};
struct Resource
{
  ResourceId id;
  QString name;
  bool texture = false;
  rdcarray<EventUsage> usages;
};
struct Access
{
  // version is the incoming version for reads, or the produced version for pure writes.
  int resource = -1, pass = -1, version = 0;
  int outputVersion = -1, producer = -1;
  uint32_t event = 0;
  ResourceUsage usage = ResourceUsage::Unused;
  AccessKind kind = AccessKind::Unknown;
};
struct Link
{
  int source = -1, destination = -1;
  // Indices into accesses: retain evidence and resource versions even when edges are bundled.
  QVector<int> evidence;
};
struct Snapshot
{
  QVector<Pass> passes;
  QVector<Resource> resources;
  QVector<Access> accesses;
  QVector<Link> links;
  std::map<uint32_t, int> eventPass;
  int externalReads = 0, unassigned = 0, unsupported = 0;
  int layers = 0, rows = 0;
};
struct TimingSummary
{
  double seconds = 0.0;
  int samples = 0, expected = 0;
};
// Only distinct leaf action EIDs count; parent/aggregate and invalid samples are excluded.
QVector<TimingSummary> SummariseTimings(const Snapshot &graph,
                                      const std::map<uint32_t, double> &seconds);
AccessKind Classify(ResourceUsage usage);
QString AccessName(AccessKind kind);
// Pure CPU construction: no replay calls, widgets or current-event mutation.
bool Build(Snapshot &graph, const std::atomic_bool &cancelled);
}

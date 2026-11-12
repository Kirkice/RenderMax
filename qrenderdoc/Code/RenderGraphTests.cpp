// Standalone CPU regression suite; deliberately independent of replay and GUI state.
#include "RenderGraph.h"
#include <cstdio>
#include <cstdlib>

static void Require(bool value, const char *message)
{
  if(!value) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
static RenderGraph::Snapshot Fixture()
{
  RenderGraph::Snapshot g;
  for(int i = 0; i < 4; ++i)
  {
    RenderGraph::Pass p; p.first = p.last = uint32_t((i+1)*10);
    p.events.push_back(p.first);
    g.passes.push_back(p); g.eventPass[p.first] = i;
  }
  RenderGraph::Resource r;
  r.usages = {EventUsage(10,ResourceUsage::CopyDst),EventUsage(20,ResourceUsage::PS_Resource),
              EventUsage(30,ResourceUsage::CopyDst),EventUsage(40,ResourceUsage::PS_Resource)};
  g.resources.push_back(r);
  return g;
}
int main()
{
  std::atomic_bool cancel(false);
  auto g = Fixture();
  Require(RenderGraph::Build(g,cancel),"build succeeds");
  Require(g.links.size() == 2,"two independent write/read chains");
  Require(g.links[0].source == 0 && g.links[0].destination == 1,"first producer");
  Require(g.links[1].source == 2 && g.links[1].destination == 3,"overwrite changes producer");
  Require(g.accesses[1].version == 1 && g.accesses[3].version == 2,"logical versions retained");
  Require(g.passes[1].layer > g.passes[0].layer,"dependency layout points forward");
  auto missing = Fixture();
  missing.resources[0].usages.push_back(EventUsage(15,ResourceUsage::CPUWrite));
  RenderGraph::Build(missing,cancel);
  Require(missing.links.size() == 1 && missing.unassigned == 1,"unknown writer invalidates producer");
  auto discarded = Fixture();
  discarded.eventPass[15] = 0;
  discarded.resources[0].usages.push_back(EventUsage(15,ResourceUsage::Discard));
  RenderGraph::Build(discarded,cancel);
  Require(discarded.links.size() == 1 && discarded.externalReads == 1,"discard invalidates contents");
  auto same = Fixture(); same.eventPass[20] = 0;
  RenderGraph::Build(same,cancel);
  Require(same.links.size() == 1,"no pass self edge");
  auto barrier = Fixture(); barrier.resources[0].usages.push_back(EventUsage(20,ResourceUsage::Barrier));
  RenderGraph::Build(barrier,cancel);
  Require(barrier.links.size() == 2,"barrier is not data producer");
  auto rw = Fixture(); rw.resources[0].usages[1].usage = ResourceUsage::CS_RWResource;
  RenderGraph::Build(rw,cancel);
  Require(rw.accesses[1].kind == RenderGraph::AccessKind::ReadWrite,"UAV access conservative");
  Require(RenderGraph::Classify(ResourceUsage::Copy) == RenderGraph::AccessKind::ReadWrite &&
          RenderGraph::Classify(ResourceUsage::Resolve) == RenderGraph::AccessKind::ReadWrite,
          "combined copy and resolve remain conservative");
  std::map<uint32_t,double> times{{10,0.001},{20,0.002},{30,-1.0},{999,1.0}};
  g.passes[0].events.push_back(10); // duplicates must not inflate duration or coverage.
  const auto summary = RenderGraph::SummariseTimings(g,times);
  Require(summary[0].samples == 1 && summary[0].expected == 1 && summary[0].seconds == 0.001,
          "timing deduplicates leaf events");
  Require(summary[1].samples == 1 && summary[1].seconds == 0.002,"timing maps reported event");
  Require(summary[2].samples == 0 && summary[2].expected == 1,"invalid timing is not fabricated");
  cancel.store(true); Require(!RenderGraph::Build(g,cancel),"cancellation");
  std::puts("PASS: overwrite, versions, unknown writer, discard, self edge, barrier, UAV, timing, cancellation");
  return 0;
}

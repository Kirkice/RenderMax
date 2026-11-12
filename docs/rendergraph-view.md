# RenderGraph View

Open **Window → RenderGraph View** or press **Ctrl+Shift+G** while an RDC capture is loaded.

## Modes

- **Frame Overview** shows non-overlapping pass cards and bundled pass-to-pass resource dependencies.
- **Pass Focus** expands one pass into producer pass → input resource → selected pass → output resource → consumer pass topology.
- **Resource History** shows the chronological accesses and logical write versions for one resource. Double-click an entry to select its event.

Search accepts pass names, marker paths, resource names, or an EID. **Include noisy bindings** is disabled by default; enable it to include constants, vertex buffers, and index buffers. Overview connections use thin lines by default; only the selected pass's incoming and outgoing connections are thickened.

Overview uses a nested dependency layout, not a rectangular event grid. Producers fan out into branches; consumers are placed near the vertical center of their producers, with collision avoidance using actual card/group sizes. Full ancestor paths share one container. Sibling groups use first-EID order to resolve placement when interleaved groups would otherwise form a cycle; all dependency edges remain available, including backward edges. Positions do not represent GPU scheduling, parallel execution, or elapsed time.

Local connections use smooth curves. Curves intersecting another card and backward connections use a lower curved bypass. Click a card to inspect it and select its primary event in EventBrowser; double-click or **Focus selected** opens the local topology. **Esc** or **← Overview** returns to the overview.

## Evidence and precision

The graph is reconstructed from `IReplayController::GetUsage(ResourceId)`. It is not an engine-authored Render Graph and it is not a GPU scheduling graph.

- Named marker paths form the human-facing pass cards. Unmarked Draw, Dispatch, Copy, and Resolve actions become explicitly inferred passes instead of being collapsed into an anonymous sequence.
- Clear commands never become top-level pass cards. They are retained inside the nearest pass on the same marker path, remain visible in the Inspector, and still participate in resource versions and dependencies. Marker parents are display containers and are not counted as additional writers.
- Every observed write creates a logical resource version. Reads connect to the latest known producer. Discard and unknown writes invalidate the previous producer.
- Attachment and UAV uses are conservative potential read/write evidence. A bound resource does not prove that a shader dynamically accessed it.
- `EventUsage` contains an EID and `ResourceUsage`, but no texture subresource or buffer byte range. The viewer therefore labels its results as resource-level precision and does not invent ranges.
- An external read means no producer was observed in the represented frame. It is not automatically an error.

## GPU timing

**GPU timing** explicitly requests `EventGPUDuration`. Only finite, non-negative samples for distinct leaf action EIDs are summed. Missing samples are shown as **Not reported**; parent marker durations and child durations are never added together. The highlighted longest measured pass is a hotspot, not a claimed GPU critical path.

## Large captures and export

Overview rendering is capped at 500 matched pass cards. Narrow the graph with search or Pass Focus when the cap is reported in the status line. Resource usages and the pure graph model are still retained.

**Export** writes the current canvas as PNG or SVG. Texture entries can be opened in Texture Viewer; buffers and other resources open in Resource Inspector.

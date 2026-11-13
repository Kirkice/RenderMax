# RenderMax Capture Replay — 0.1.0

Unity 2019.4 Editor package. Main entry: **Tools > RenderMax > Replay Tools**.

The unified window has two tabs:
- **Capture / Import**: choose a manifest, verify evidence and import into a NEW folder.
- **Scene / Display**: select Capture.asset, create the all-draw scene, replace/rebind generated materials (GUID-preserving backups), recover the main camera, toggle EID order/SceneView and Gamma display.

Scene controls require exactly one active CaptureReplayController. Scene/display edits support Undo and mark the scene dirty without auto-saving; material replacement has its own backup and saved-scene requirements. Camera recovery uses the selected capture, so select the capture matching the open scene.

Experimental single-draw/E63 tools, legacy Standard repairs and tests remain under **Tools > RenderMax > Advanced**. They are not normal pipeline steps. No existing shader, asset GUID or runtime script identity is changed by this menu reorganization.

## Usage

1. Select the exported `manifest.json`.
2. Click **Read Package**, then **Verify All Files (SHA-256)**.
3. Choose a **new** folder such as `Assets/RenderMaxCaptures/Capture01`.
4. Click **Validate & Import Evidence Assets**.
5. Inspect `Capture.asset` and `ImportReport.json`.

The importer never overwrites an existing destination. Failed/cancelled imports are retained with a report; select another new directory to retry. Validation and copying currently run synchronously with a cancelable progress bar.

## Experimental generated-shader material upgrade

For the existing Ying_Ultra / Capture01 candidates, select `Capture.asset`, then run
**Tools > RenderMax > Phase 2 > Replace Capture Materials With Generated Shaders**.
Save modified capture scenes/materials and leave Play mode before running.

- Uses `Editor/Shaders/Phase2/CaptureCandidates/SceneShaderMap.json` to map 104 event IDs to 42 generated editable shader variants. This is a capture-specific map, not a general shader translator.
- Updates mapped `E<eventId>.mat` Standard/Phase2 materials in place (GUIDs remain unchanged); leaves unmapped/custom materials alone.
- Synchronously checks all used shaders and prepares constants/textures before changing existing assets. Unsupported shaders abort the upgrade; resource limitations are written to the report.
- Backs up materials, capture scenes and their `.meta` files to `Artifacts/RenderMaxMaterialUpgrade/<run>/` before replacement. A partial write failure is reported; restore the backed-up files to their original project-relative paths if needed.
- Stores vector/matrix arrays in `GeneratedMaterialBindings` material sub-assets; `GeneratedMaterialState` restores them after scene/domain reload. Scalars/vectors/textures remain editable material properties.
- Updates renderers in capture scenes, disables old Standard property-block drivers and preserves mesh/transforms/material references.
- Also persists array bindings in newly created full Shader Trial scenes.

These shaders use live Unity clip positions with other captured constants. Missing Cubemap/DDS resources, sampler fidelity and full-frame dependencies remain unresolved. A successful upgrade is not visual/pixel equivalence. The original Phase 1 scene builder still creates Standard materials; use this upgrade explicitly afterward.

## Implemented

- Version/status validation, package-root path restrictions, duplicate paths, size and SHA-256 validation.
- Draw inventory, pipeline/constants/bindings references, texture and buffer references.
- All original listed files copied into `Evidence~` (ignored by Unity asset importing).
- Mesh CSV/JSON to Mesh.asset: position, normal, tangent, color and UV0–UV7; triangle/line/point lists; index range checks; invariant-culture numeric parsing.
- No coordinate conversion, vertex optimization or normal/tangent regeneration.
- PNG/TGA/EXR preview assets with explicit sRGB, unchanged alpha, no automatic mip generation, no compression or NPOT rescaling.
- Imported texture dimension checks.
- Source linkage, unsupported-feature reporting, import report and six menu-driven self tests.

## Deliberately not claimed

This version imports **evidence and previews**, not a functioning replay scene.

- Independent mip images are not yet assembled into the exact original GPU mip chain.
- DDS Cubemap, texture arrays/volumes, depth targets and MSAA need dedicated GPU resource creation.
- Mesh.asset is a convenience representation, not byte-exact vertex-buffer reconstruction. Unmapped and instance attributes remain in the original evidence.
- Custom vertex inputs require explicit semantic mapping; missing POSITION is not silently fabricated.
- Original buffers are copied, not yet bound to GPU resources.
- Shader translation, constant buffer and sampler binding, pipeline recreation, CommandBuffer execution and pixel comparisons are pending.
- Texture importer defaults for sampling are not capture sampler reconstruction.
- No Unity rendering equivalence has been established.

## Development checks

Sources compiled successfully using this installation's Unity 2019 C# compiler and UnityEngine/UnityEditor references. To execute tests inside the running editor:

**Tools > RenderMax > Run Import Self Tests**

Tests cover CSV escaping, malformed CSV, valid checksums, traversal, tampering and incomplete package rejection. The real capture package must additionally be imported and inspected in Unity.

## Next milestones

1. Exact texture mip-chain assembly and DDS Cubemap conversion.
2. Raw GPU vertex/index/constant/resource buffer upload.
3. Explicit shader/semantic/binding mapping and first draw reconstruction.
4. Capture pipeline state application and standalone CommandBuffer replay.
5. Color/depth readback, numerical comparison and first-divergent-event diagnostics.
6. Pass/full-frame dependency replay after exporter adds pre-action states and resource-changing operations.

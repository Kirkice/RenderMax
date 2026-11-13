# Reliable ASTC preview export (OpenGL capture)

The native frame exporter preserves compressed texture bytes with GetTextureData,
but SaveTexture uses a GPU conversion path. A previous Ying_Ultra export produced
all-zero PNGs for nonzero ASTC payloads. Identical compressed payloads exported at
other events produced valid images. CPU decoding verified those raw bytes.

Until the native GPU path is fixed, make CPU normalization a required export step:

```text
1. renderdoc_openCapture {"filePath":".../Ying_Ultra.rdc"}
2. renderdoc_exportFrameAssets {"outputPath":".../Ying_Ultra.rmasset"}
3. python util/repair_rmasset_astc.py .../Ying_Ultra.rmasset --origin opengl
4. python util/validate_rmasset.py .../Ying_Ultra.rmasset
5. Import the validated package into a NEW Unity destination.
```

Dependencies: Pillow, numpy, texture2ddecoder. The repair preserves raw evidence,
backs up previews/metadata/manifest outside the package, CPU-decodes every ASTC
image (not only black images), preserves alpha, and updates manifest hashes/sizes.
The footprint is inferred from exact compressed byte sizes across ALL mips, never
from the ASTC format enum alone. Ambiguous non-equivalent decodes fail closed.
An explicit --block requires independent knowledge of the capture format.

`--origin opengl` flips raw rows to match SaveTexture's exported orientation;
use `--origin top-left` only after checking the capture API/orientation. The repair
supports LDR ASTC_UNORM 2D single-sample subresources, not HDR ASTC or 3D ASTC.
It does not reconstruct cubemaps, shadow resources, pass dependencies or cameras.
A zero-black repair count is legitimate: the GPU failure can be intermittent.
Inspect the report, not the black count alone.

Validation for Ying_Ultra (2026-11-13): the original E126_T3686 PNG was all zero;
CPU decode of its ASTC 6x6 data matched the valid E1080_T3686 PNG after vertical
flip (mean absolute channel difference 0.0237 / 255). A fresh export decoded all
135 ASTC image subresources without a black result, maximum CPU/GPU mean absolute
difference 0.0625 / 255. File-integrity validation passed all 2238 files.

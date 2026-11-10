# RenderMax System Trace (.rdt)

Use **Save Trace As...** in the System Trace panel after opening or recording an importable trace. Choose a `.rdt` destination. Reopen through **Open Trace...** or the main **Open Capture** dialog's RenderMax System Trace filter. Opening an RDT does not close the active RDC.

RDT v1 is a self-contained, uncompressed archive: magic `RenderMaxRDT\r\n` (including terminating NUL), big-endian uint32 version, followed by four uint64-length-prefixed byte arrays: UTF-8 JSON metadata, original importable timeline bytes, optional original Perfetto bytes, and SHA-256(metadata + timeline + Perfetto). Each part is limited to 512 MiB. Unsupported versions, truncated lengths, nested archives and checksum failures are rejected. QSaveFile commits atomically. Timeline reimport uses a temporary file removed after parsing; the original source path is not required.

Capture output's sibling `capture.perfetto-trace`, when present, is attached. A trace imported from elsewhere may not have that attachment. RDT stores source data rather than a lossy serialization of the model. Viewing preferences, zoom, annotations and capture warnings not present in the source are not persisted in v1. External tools must use original Perfetto/source files, not open RDT directly. A dedicated attachment export UI is not implemented yet.

## MCP

`renderdoc_saveSystemTrace`:

```json
{"filePath":"C:/traces/capture.systrace.txt","destinationPath":"C:/traces/session.rdt","overwrite":false}
```

Both paths must be absolute; source must be valid and destination must end in `.rdt`. Existing destinations require `overwrite:true`. Source and destination cannot be identical. All SystemTrace analysis tools accept RDT through the shared importer, including capture correlation. This does not turn RDT into a replay capture or provide GPU hardware counters absent from the original trace.

Saving currently reads/validates the source synchronously; very large traces can block the UI during save. File association / Windows Explorer installation integration is not included.

# RenderMax CLI — independent Trace and Capture execution

The default CLI no longer requires an interactive RenderMax process or an enabled MCP server.
It owns its replay/target session and calls the same tool registry directly. HTTP/MCP is an
**optional** backend, selected only by `--backend instance`.

## Build and run

Python 3.8+ (stdlib only), the native runtime and its normal DLL dependencies are required.

```bat
rem Build the application/runtime using the normal RenderMax build.
rem Build the small offline Trace helper from an x64 VS Native Tools prompt:
util\cli\build_windows.bat

util\cli\rendermax-cli.cmd tools list
util\cli\rendermax-cli.cmd trace gpu H:/traces/sample.rdt
util\cli\rendermax-cli.cmd --capture H:/captures/sample.rdc call getReplayStatus
util\cli\rendermax-cli.cmd --allow-sensitive-data --capture H:/captures/sample.rdc call getPipelineState
```

POSIX launcher: `util/cli/rendermax-cli`. Python entry: `util/cli/rendermax_cli.py`.
The separate Trace helper also has a standalone CMake project under `util/cli`.

### Runtime implementation

For now, the native headless runtime is shipped **inside the existing qrenderdoc executable**
as its early `--cli-headless` entry point. The CLI starts this entry directly, using
`QCoreApplication`: **no QApplication, MainWindow, Python interpreter, HTTP listener,
MCP initialize/session or UI.config enable flag**. This is not a hidden GUI or remote control
of an existing UI process. It does still link the existing executable's Qt Widgets/GUI DLLs;
it is not yet a separately packaged, minimal QtCore-only replay binary.

The runtime constructs `CaptureContext(config, true)`, ReplayManager and the shared tool registry.
It reads/writes private request/result files in an OS-created temporary directory. No listening
port is opened. Interactive settings/extensions are not loaded. Sensitive data access requires
`--allow-sensitive-data`, not a change to the GUI's MCP configuration.

## Backend selection

| Backend | Execution |
|---|---|
| auto (default) | 28 file-side Trace tools use the offline helper; all others use independent native runtime |
| local | Only the 28 file-side Trace tools; QtCore helper |
| headless | Independent native runtime, all 111 registered tools |
| instance | Explicitly connect to an existing GUI/MCP instance; legacy optional mode |

`tools list` defaults to independent headless discovery of all 111 tools.
`tools describe NAME` returns the shared authoritative schema. `call NAME` accepts either
`renderdoc_...` or the short name. There is no hardcoded Capture tool whitelist.

Global options must precede the command:

* `--runtime PATH` / `RENDERMAX_RUNTIME`: native executable override; default source-tree
  `x64/Development/qrenderdoc.exe` on Windows.
* `--trace-backend PATH` / `RENDERMAX_TRACE_BACKEND`: standalone Trace helper override.
* `--capture FILE`: open a local capture and run one tool within the same independent process.
* `--allow-sensitive-data`: permit existing sensitive resource/shader/content tools.
* `--timeout-ms N`: default 120000. Bounds each native invocation or HTTP exchange, not discovery
  plus all subsequent invocations together. Timeout terminates the independent process; already
  submitted device operations may still complete. No automatic side-effect retries.
* `--output FILE`: write successful JSON to a file instead of stdout.
* `--endpoint URL` / `RENDERMAX_ENDPOINT`: only used by instance backend; default localhost:38967/mcp.

## Sessions: single command versus batch

Each independent invocation owns and releases its own connections. A separate `capture open`
command cannot keep replay loaded for a later shell command. Use `--capture FILE` for single
queries or **batch** for stateful workflows. Batch does not require a daemon or GUI.

`analysis.json`:

```json
[
  {"name":"openCapture", "arguments":{"filePath":"H:/captures/sample.rdc"}},
  {"name":"getReplayStatus"},
  {"name":"enumerateCounters"},
  {"name":"getFrameSummary"},
  {"name":"getPipelineState", "arguments":{"eventId":100}}
]
```

```bat
rendermax-cli --allow-sensitive-data --output analysis-result.json batch analysis.json
```

The complete plan is schema-checked before execution. Steps execute in order in one runtime;
the first tool failure or business failure (`ok:false` / `success:false`) stops later steps.
Output includes each executed step/result. Batch is **not a transaction**: earlier side effects
are not rolled back. `--capture` and batch cannot be combined; put openCapture in the plan.
Batch argument values are literal; there is currently no previous-result interpolation.

An Android recording plan:

```json
[
  {"name":"launchRemoteApplication", "arguments":{
    "targetUrl":"adb://DEVICE_SERIAL",
    "packageActivity":"com.example.app/com.example.MainActivity"}},
  {"name":"waitForLiveTarget", "arguments":{"timeoutMs":10000}},
  {"name":"captureSystemTrace", "arguments":{"durationMs":5000,"gpuDetail":"commands"}}
]
```

Launch authority includes existing Android server setup/install/version checks; CLI does not
bypass them. Matching server APKs must be deployed under the runtime's `plugins/android`.
Hardware counters require remote/device replay, not local desktop replay. Use connectRemoteReplay
and its capturePath in one batch with counter queries. Remote replay remains single-client:
headless never steals a connection from another client. Close an existing GUI/device replay first.

A batch finishes by releasing its own replay and target connections. It never closes another
instance's session or kills the launched target application merely because the CLI exits.
For command-to-command persistence beyond batch, use explicit instance mode; there is currently
no persistent headless daemon.

## Arguments and results

```bat
rendermax-cli call analyzeSystemTraceGpu --args-file query.json
rendermax-cli trace gpu H:/traces/sample.rdt --set beginSec=10 --set endSec=11
rendermax-cli trace frame H:/traces/sample.rdt --set frameIndex=0
rendermax-cli trace save H:/traces/source.json --args-file destination.json
```

* `--args JSON` or `--args-file FILE` (mutually exclusive); `--args-file -` reads stdin.
* `--set KEY=JSON` overrides an argument. String values need JSON quoting; use argument files on
  Windows to avoid shell quoting differences. Native schema keys are camelCase.
* Friendly Trace positional file paths and `--capture` resolve to absolute paths. Raw JSON paths
  are not rewritten; supply existing absolute paths where required.
* stdout is success JSON only; logs/errors go to stderr. Native capture logs may also be written
  by the replay core in its standard log directory.
* Unknown data stays null; empty counter catalogs are not measurements.
* No implicit data pagination. Supply the tool's limit/offset/cursor; discovery pagination in
  optional instance mode is automatic.
* Exit codes: 0 success, 1 general/business failure, 2 bad input, 3 no capture,
  4 replay/connection unavailable, 5 timeout, 6 interrupted/cancelled, 7 unsupported/backend missing.
  Batch business failures currently use 1, with native error details in stderr JSON.
* Ctrl+C interrupts the independent invocation; submitted device work may continue.
  Instance mode does not currently send remote cancellation notifications.

For save: `destination.json` can contain
`{"destinationPath":"H:/traces/saved.rdt","overwrite":false}`. Existing destinations require
explicit overwrite=true; overwriting the source itself is always rejected.

## Friendly aliases

| Group | Commands |
|---|---|
| trace overview | info, analyze, events, series, tracks |
| trace CPU | cpu, cpu-utilization, thread-times, scheduling, wakeups, cpu-frequency, cpu-idle |
| trace GPU | gpu, gpu-utilization, gpu-batches, gpu-queue, gpu-waits, gpu-frequency |
| trace other | bandwidth, power, thermal, power-states, frames, frame, jank, compare, report, save, record, correlate |
| session | targets, check-android, launch, launch-windows, launch-android, connect-replay, wait-target, wait-capture, close |
| capture | open, frame, remote-frame |
| call | Every registered tool, including Capture analysis, shader/resource/counter/asset tools |

A display-only tool such as showUnityShader returns its read-only panel content with
`displayed:false` in headless mode rather than constructing a viewer. Headless replay is data
analysis, not GUI screenshot/interactive viewer automation.

## Verification

```bat
python -m unittest discover -s util/cli -p test_cli.py -v
set RENDERMAX_TEST_CAPTURE=H:/captures/sample.rdc
python -m unittest discover -s util/cli -p test_cli.py -v
```

The suite covers local Trace tools, schema/error behavior, archive roundtrip/corruption, optional
MCP SSE/session transport, native tool discovery, fail-fast batching and optional real-file replay.
Set RENDERMAX_TEST_ENDPOINT to explicitly opt into GUI/MCP parity tests.

Validated independently on Windows: all 111 tools discovered, real OpenGL capture open/status,
counter enumeration, frame summary and sensitive pipeline query. This does not assert all 111
side effects or all graphics APIs have been exercised. Android launch integration currently hit
`REMOTE_SERVER_FAILED: Failed to verify installed Android remote server` with the local test
APKs; no version checks were weakened to hide this deployment failure.

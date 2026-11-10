# CLI tool coverage matrix

Measured from the shared registry: **111 tools**, including **30 SystemTrace**. Earlier static count 96 was incomplete. All tools now have independent headless execution through generic `call`; 28 additionally support the small offline Trace helper. In the Backend column below, `instance` is the optional legacy connection route, not a requirement: every such row also supports the independent headless runtime. Contracts and authorization remain shared.

`tools describe NAME` returns the complete current schema/annotations. Rows below are a discovery snapshot, not a second registration list. Read-only annotations are server declarations, not a guarantee of no runtime work.

| Tool | Backend | Declared read-only | Required arguments |
|---|---|---|---|
| renderdoc_addBookmark | instance | false | title |
| renderdoc_analyzeFrame | instance | true |  |
| renderdoc_analyzeHotEvent | instance | true |  |
| renderdoc_analyzeMaliShader | instance | true | stage |
| renderdoc_analyzeSystemTrace | local / instance | true | filePath |
| renderdoc_analyzeSystemTraceBandwidth | local / instance | true | filePath |
| renderdoc_analyzeSystemTraceCpu | local / instance | true | filePath |
| renderdoc_analyzeSystemTraceCpuFrequency | local / instance | true | filePath |
| renderdoc_analyzeSystemTraceCpuIdle | local / instance | true | filePath |
| renderdoc_analyzeSystemTraceFrame | local / instance | true | filePath |
| renderdoc_analyzeSystemTraceGpu | local / instance | true | filePath |
| renderdoc_analyzeSystemTraceGpuFrequency | local / instance | true | filePath |
| renderdoc_analyzeSystemTraceGpuQueue | local / instance | true | filePath |
| renderdoc_analyzeSystemTraceGpuWaits | local / instance | true | filePath |
| renderdoc_analyzeSystemTraceJank | local / instance | true | filePath |
| renderdoc_analyzeSystemTracePower | local / instance | true | filePath |
| renderdoc_analyzeSystemTracePowerStates | local / instance | true | filePath |
| renderdoc_analyzeSystemTraceScheduling | local / instance | true | filePath |
| renderdoc_analyzeSystemTraceThermal | local / instance | true | filePath |
| renderdoc_analyzeSystemTraceWakeups | local / instance | true | filePath |
| renderdoc_applyShaderEdit | instance | false | resourceId, shaderStage, sourceEncoding, entryPoint, files |
| renderdoc_bookmarksExtended | instance | true |  |
| renderdoc_buildEventBrowserContext | instance | true |  |
| renderdoc_buildInvestigationReport | instance | true |  |
| renderdoc_buildSystemTraceReport | local / instance | true | filePath |
| renderdoc_captureFrame | instance | false |  |
| renderdoc_captureSystemTrace | instance | false |  |
| renderdoc_checkAndroidLaunchReadiness | instance | true | packageName |
| renderdoc_closeSession | instance | false |  |
| renderdoc_compareCaptures | instance | true | baselinePath, candidatePath |
| renderdoc_compareEventTimings | instance | true | baselinePath, candidatePath |
| renderdoc_compareResourceMemory | instance | true | baselinePath, candidatePath |
| renderdoc_compareShaders | instance | true | leftEventId, rightEventId |
| renderdoc_compareSystemTraceWindows | local / instance | true | filePath |
| renderdoc_connectRemoteReplay | instance | false |  |
| renderdoc_correlateSystemTrace | instance | true |  |
| renderdoc_diagnoseEnvironment | instance | false |  |
| renderdoc_diffPipelineState | instance | true |  |
| renderdoc_enumerateCounters | instance | true |  |
| renderdoc_exportDrawAssets | instance | false | outputPath |
| renderdoc_exportFrameAssets | instance | false | outputPath |
| renderdoc_exportInvestigationReport | instance | false |  |
| renderdoc_exportPerformanceReport | instance | true |  |
| renderdoc_fetchCounters | instance | true |  |
| renderdoc_findDrawsByResourceId | instance | true |  |
| renderdoc_findDrawsByShader | instance | true |  |
| renderdoc_findDrawsByTexture | instance | true |  |
| renderdoc_findProjectImplementation | instance | true | projectRoots |
| renderdoc_findResourceLeaks | instance | true |  |
| renderdoc_findShaderVariants | instance | true | shaderName |
| renderdoc_findUnusedResources | instance | true |  |
| renderdoc_generatePerformanceReport | instance | false |  |
| renderdoc_getActionTimings | instance | true |  |
| renderdoc_getBoundResources | instance | true | eventId |
| renderdoc_getBufferContents | instance | true |  |
| renderdoc_getBufferDetail | instance | true |  |
| renderdoc_getBuffers | instance | true |  |
| renderdoc_getCaptureInfo | instance | true |  |
| renderdoc_getCurrentDrawPreview | instance | true | eventId |
| renderdoc_getDrawCalls | instance | true |  |
| renderdoc_getEnvironmentDiagnostics | instance | true |  |
| renderdoc_getEventChunks | instance | true |  |
| renderdoc_getEventDetails | instance | true |  |
| renderdoc_getFrameSummary | instance | true |  |
| renderdoc_getHotEvents | instance | true |  |
| renderdoc_getMaliToolStatus | instance | true |  |
| renderdoc_getMeshData | instance | true |  |
| renderdoc_getPassGraph | instance | true |  |
| renderdoc_getPipelineState | instance | true |  |
| renderdoc_getPipelineStateAtEvent | instance | true |  |
| renderdoc_getReplayStatus | instance | true |  |
| renderdoc_getResourceDetail | instance | true |  |
| renderdoc_getResourceLifetime | instance | true |  |
| renderdoc_getResources | instance | true |  |
| renderdoc_getSelectionContext | instance | true |  |
| renderdoc_getSessionState | instance | true |  |
| renderdoc_getShaderCompileDiagnostics | instance | true | eventId |
| renderdoc_getShaderEntryPoints | instance | true |  |
| renderdoc_getShaderInfo | instance | true |  |
| renderdoc_getShaderSource | instance | true | eventId |
| renderdoc_getSystemTraceCapabilities | local / instance | true | filePath |
| renderdoc_getSystemTraceCpuUtilization | local / instance | true | filePath |
| renderdoc_getSystemTraceEventInventory | local / instance | true | filePath |
| renderdoc_getSystemTraceFrames | local / instance | true | filePath |
| renderdoc_getSystemTraceGpuBatches | local / instance | true | filePath |
| renderdoc_getSystemTraceGpuUtilization | local / instance | true | filePath |
| renderdoc_getSystemTraceThreadTimes | local / instance | true | filePath |
| renderdoc_getTextureData | instance | true |  |
| renderdoc_getTextureInfo | instance | true |  |
| renderdoc_getTextures | instance | true |  |
| renderdoc_launchApplication | instance | false | app |
| renderdoc_launchRemoteApplication | instance | false | packageActivity |
| renderdoc_launchWindowsApplication | instance | false | executablePath |
| renderdoc_listBookmarks | instance | true |  |
| renderdoc_listCaptureTargets | instance | true |  |
| renderdoc_mapProjectSource | instance | true | sourceReference |
| renderdoc_openCapture | instance | false |  |
| renderdoc_prepareUnityShaderConversion | instance | true | eventId |
| renderdoc_querySystemTraceSeries | local / instance | true | filePath |
| renderdoc_querySystemTraceTracks | local / instance | true | filePath |
| renderdoc_removeBookmark | instance | false |  |
| renderdoc_resourceLifetime | instance | true |  |
| renderdoc_resourceMemoryAudit | instance | true |  |
| renderdoc_saveSystemTrace | local / instance | false | filePath, destinationPath |
| renderdoc_showUnityShader | instance | true | shader |
| renderdoc_traceResourceUsage | instance | true |  |
| renderdoc_triggerRemoteCapture | instance | false |  |
| renderdoc_updateBookmark | instance | false |  |
| renderdoc_validateShaderEdit | instance | true | resourceId, shaderStage, sourceEncoding, entryPoint, files |
| renderdoc_waitForCapture | instance | true |  |
| renderdoc_waitForLiveTarget | instance | true |  |

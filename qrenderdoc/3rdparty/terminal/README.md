# Embedded terminal dependencies

- xterm.js 5.5.0: https://www.npmjs.com/package/@xterm/xterm/v/5.5.0
  MIT license, see xterm-LICENSE.txt. Only the distribution JS/CSS is included.
  Local patch: the DOM renderer's 24-bit foreground branch applies SGR 2 (dim)
  by halving RGB channels, matching Windows Terminal. Upstream's inline RGB color
  overrides its generated `.xterm-dim` CSS, leaving truecolor dim text undimmed.
  The patch is limited to the RGB branch; indexed colors retain upstream behavior.
- Microsoft.Windows.Console.ConPTY 1.24.261001001:
  https://www.nuget.org/packages/Microsoft.Windows.Console.ConPTY/1.24.261001001
  x64 conpty.dll and OpenConsole.exe, MIT license, see conpty/LICENSE.txt.
  Use this console host rather than the Windows 10 inbox version, which rewrites
  SGR dim and consumes device-attribute queries, breaking TUI color/protocol parity.
- Microsoft WebView2 SDK 1.0.2903.40: https://www.nuget.org/packages/Microsoft.Web.WebView2/1.0.2903.40
  Header and x64 loader DLL, see WebView2-LICENSE.txt.

The application embeds the local HTML/JS/CSS via Qt resources. It does not download terminal
code at runtime. WebView2 Evergreen Runtime must be installed on Windows. ConPTY requires
Windows 10 version 1809 or later. There is no conhost-window embedding or redirected-text fallback.

The x64 loader is copied alongside the executable by the local MSBuild target. Other packaging
paths must also include WebView2Loader.dll, conpty.dll and OpenConsole.exe alongside
qrenderdoc.exe. Non-x64 Windows packaging is not yet supported.

Terminal output uses bounded 32KB messages and waits for the renderer's write acknowledgment
before reading another chunk. Input is queued to a background pipe writer, not the GUI thread.

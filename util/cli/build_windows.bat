@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  echo Run from an x64 Visual Studio Native Tools command prompt. 1>&2
  exit /b 1
)
set "ROOT=%~dp0..\.."
set "QT=%ROOT%\qrenderdoc\3rdparty\qt"
set "OUT=%ROOT%\x64\Development"
if not exist "%OUT%" mkdir "%OUT%"
pushd "%TEMP%"
cl /nologo /EHsc /MD /std:c++14 /utf-8 /W3 "%~dp0trace_backend.cpp" "%ROOT%\qrenderdoc\Code\SystemTrace.cpp" /I "%ROOT%\qrenderdoc" /I "%QT%\include" /I "%QT%\include\QtCore" /Fe:"%OUT%\rendermax-trace.exe" /link /LIBPATH:"%QT%\x64\lib" Qt5Core.lib
set "RESULT=%ERRORLEVEL%"
popd
if not "%RESULT%"=="0" exit /b %RESULT%
if not exist "%OUT%\Qt5Core.dll" copy /Y "%QT%\x64\bin\Qt5Core.dll" "%OUT%\Qt5Core.dll" >nul
exit /b 0

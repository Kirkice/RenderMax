@echo off
python "%~dp0rendermax_cli.py" %*
exit /b %ERRORLEVEL%

@echo off
setlocal

IF %PROCESSOR_ARCHITECTURE%==x86 (
  SET asm="C:\Program Files\Common Files\ArcGIS\bin\ESRIRegAsm.exe"
) ELSE (
  SET asm="C:\Program Files (x86)\Common Files\ArcGIS\bin\ESRIRegAsm.exe"
)

SET bin="%~dp0bin\EvcSolver.dll"
if not exist %asm% (goto :nofile)
if not exist %bin% (goto :nofile)

%asm% /p:desktop %bin%

goto :end

:nofile
echo File not found

:end
IF %ERRORLEVEL% == 0 ( echo "No error found" ) else ( echo "Error found" )
endlocal

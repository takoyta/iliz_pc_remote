@echo off
setlocal EnableDelayedExpansion
set ROOT=%~dp0

call "%ROOT%build.bat" || exit /b 1

rem Universal updater (separate project). Override UPDATER_DIR / UPDATER_EXE if it lives elsewhere.
if not defined UPDATER_DIR set UPDATER_DIR=%ROOT%..\iliz_updater
if not defined UPDATER_EXE set UPDATER_EXE=%UPDATER_DIR%\bin\iliz_updater.exe
if not exist "%UPDATER_EXE%" if exist "%UPDATER_DIR%\build.bat" call "%UPDATER_DIR%\build.bat" || exit /b 1
if not exist "%UPDATER_EXE%" (
  echo Missing %UPDATER_EXE% - build ..\iliz_updater or set UPDATER_EXE
  exit /b 1
)
if not exist "%ROOT%defaults\iliz_updater.json" (
  echo Missing defaults\iliz_updater.json
  exit /b 1
)

if not exist "%ROOT%bin\iliz_pc_remote.exe" (
  echo Missing bin\iliz_pc_remote.exe
  exit /b 1
)
if not exist "%ROOT%bin\bass.dll" (
  echo Missing bin\bass.dll — put bass.dll next to the exe in bin\
  exit /b 1
)
if not exist "%ROOT%bin\bassopus.dll" (
  echo Missing bin\bassopus.dll — put bassopus.dll next to the exe in bin\
  exit /b 1
)
if not exist "%ROOT%defaults\config.json" (
  echo Missing defaults\config.json
  exit /b 1
)
if not exist "%ROOT%defaults\command_help.md" (
  echo Missing defaults\command_help.md
  exit /b 1
)

set VER=
for /f "tokens=3 delims= " %%a in ('findstr /C:"#define APP_VERSION_A" "%ROOT%src\appinfo.h"') do set VER=%%~a
set VER=!VER:"=!
if "!VER!"=="" (
  echo Could not read APP_VERSION_A from src\appinfo.h
  exit /b 1
)

set OUT=%ROOT%dist
if not exist "%OUT%" mkdir "%OUT%"
set NAME=iliz_pc_remote-!VER!
set STAGE=%OUT%\%NAME%
set ZIP=%OUT%\%NAME%.zip

if exist "%STAGE%" rmdir /s /q "%STAGE%"
if exist "%ZIP%" del /q "%ZIP%"
mkdir "%STAGE%"
copy /y "%ROOT%bin\iliz_pc_remote.exe" "%STAGE%\" >nul
copy /y "%ROOT%bin\bass.dll" "%STAGE%\" >nul
copy /y "%ROOT%bin\bassopus.dll" "%STAGE%\" >nul
copy /y "%ROOT%defaults\config.json" "%STAGE%\config.json" >nul
copy /y "%ROOT%defaults\command_help.md" "%STAGE%\command_help.md" >nul
if exist "%ROOT%bin\beep.mp3" copy /y "%ROOT%bin\beep.mp3" "%STAGE%\" >nul
copy /y "%UPDATER_EXE%" "%STAGE%\iliz_updater.exe" >nul || exit /b 1
copy /y "%ROOT%defaults\iliz_updater.json" "%STAGE%\iliz_updater.json" >nul || exit /b 1

powershell -NoProfile -Command "Compress-Archive -Path '%STAGE%\*' -DestinationPath '%ZIP%' -Force" || exit /b 1
rmdir /s /q "%STAGE%"

rem Optional SHA256SUMS (sha256sum format). The updater uses GitHub's asset digest; this file is only
rem needed for non-GitHub hosting. Set NO_SUMS=1 to skip. A failure here does not fail the release.
if defined NO_SUMS goto :sums_done
powershell -NoProfile -Command "$f=Get-Item '%ZIP%'; $stream=[IO.File]::OpenRead($f.FullName); try { $h=([BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash($stream))).Replace('-','').ToLower() } finally { $stream.Dispose() }; [IO.File]::WriteAllText('%OUT%\SHA256SUMS', $h + '  ' + $f.Name + [char]10)" || echo Warning: could not write SHA256SUMS
:sums_done

echo Packed: %ZIP%
if not defined NO_SUMS echo Sums:   %OUT%\SHA256SUMS

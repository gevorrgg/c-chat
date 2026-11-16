@echo off
rem Configure and build c-chat on Windows.
rem
rem Usage:
rem   build                 Debug build into .\build
rem   build release         Release build
rem   build clean           remove .\build and rebuild from scratch
rem   build release clean   any combination
rem
rem Dependencies (libsodium) are fetched by CMake automatically. If cmake
rem or git are missing, the script installs them with winget. A C compiler
rem (Visual Studio Build Tools or MinGW) must be installed separately.

setlocal EnableDelayedExpansion

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "BUILD_DIR=%ROOT%\build"
set "BUILD_TYPE=Debug"
set "CLEAN=0"

:parse
if "%~1"=="" goto parsed
if /I "%~1"=="release" set "BUILD_TYPE=Release"
if /I "%~1"=="debug"   set "BUILD_TYPE=Debug"
if /I "%~1"=="clean"   set "CLEAN=1"
if /I "%~1"=="-h"      goto help
if /I "%~1"=="--help"  goto help
shift
goto parse

:help
echo Usage: build [debug^|release] [clean]
exit /b 0

:parsed
call :ensure cmake Kitware.CMake  || exit /b 1
call :ensure git   Git.Git         || exit /b 1
goto :main

rem ensure <exe> <winget id> - installs the package with winget if <exe> is missing
:ensure
where %1 >nul 2>nul && exit /b 0
echo ==^> '%1' not found, trying to install it with winget
where winget >nul 2>nul || (
    echo error: winget is not available, install %1 manually
    exit /b 1
)
winget install --id %2 -e --accept-source-agreements --accept-package-agreements || exit /b 1
rem winget updates PATH for new shells only, so pick up the usual install locations now
set "PATH=%PATH%;%ProgramFiles%\CMake\bin;%ProgramFiles%\Git\cmd"
where %1 >nul 2>nul || (
    echo error: %1 was installed but is not in PATH yet, open a new terminal and run build again
    exit /b 1
)
exit /b 0

:main

if "%CLEAN%"=="1" if exist "%BUILD_DIR%" (
    echo ==^> Removing %BUILD_DIR%
    rmdir /s /q "%BUILD_DIR%"
)

echo ==^> Configuring (%BUILD_TYPE%)
cmake -S "%ROOT%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% || exit /b 1

echo ==^> Building
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% -j %NUMBER_OF_PROCESSORS% || exit /b 1

echo.
echo Done. Binaries are under %BUILD_DIR%\client and %BUILD_DIR%\server
endlocal

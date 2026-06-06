@echo off
set "VCVARS_PATH="

if exist "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    goto :found
)
if exist "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS_PATH=C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
    goto :found
)

if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" goto :found
for /f "usebackq tokens=*" %%i in (`^""%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -find **\vcvars64.bat^"`) do (
    set "VCVARS_PATH=%%i"
)

:found
if not "%VCVARS_PATH%"=="" (
    call "%VCVARS_PATH%" > nul 2>&1
)

set PATH=%~dp0..\ninja;%PATH%
cd /d "%~dp0"
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug

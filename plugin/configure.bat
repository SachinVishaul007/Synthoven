@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1
set PATH=%~dp0ninja;%PATH%
cd /d "%~dp0"
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug


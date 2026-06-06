@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1
set PATH=C:\Users\sachi\tools\ninja;%PATH%
cd /d "C:\Users\sachi\Synthoven\plugin"
cmake --build build --config Debug

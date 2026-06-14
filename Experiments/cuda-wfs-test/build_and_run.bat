@echo off
rem Build + run the CUDA WFS kernel validation harness.
rem nvcc needs cl.exe on PATH, so enter the VS2022 x64 dev environment first.
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0"
nvcc -o test_wfs_kernels.exe test_wfs_kernels.cpp -lnvrtc -lcuda -Wno-deprecated-gpu-targets
if errorlevel 1 exit /b 1
test_wfs_kernels.exe

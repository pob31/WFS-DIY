@echo off
rem Build + run the CUDA SDN backend correctness/timing harness (Windows/NVIDIA).
rem nvcc needs cl.exe on PATH, so enter the VS2022 x64 dev environment first.
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0"
nvcc -std=c++17 -O2 -DWFS_GPU_NATIVE=1 -I..\..\Source\DSP\gpu ^
    -o backend_test.exe backend_test.cpp ..\..\Source\DSP\gpu\CudaSdnBackend.cpp ^
    -lnvrtc -lcuda -lcudart -Wno-deprecated-gpu-targets
if errorlevel 1 exit /b 1
backend_test.exe

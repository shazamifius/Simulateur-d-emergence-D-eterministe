@echo off
setlocal EnableDelayedExpansion

echo ========================================================
echo   SED LAB - INSTALLER (CPU DETERMINISTIC)
echo ========================================================
echo.

:: 1. Check Prerequisites
echo [1/5] Checking System Prerequisites...
where git >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Git is not found! Please install Git.
    pause
    exit /b 1
)

where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake is not found! Please install CMake.
    pause
    exit /b 1
)

:: 2. Setup VCPKG
echo.
echo [2/5] Setting up Dependency Manager (vcpkg)...
if not exist "vcpkg\vcpkg.exe" (
    if not exist "vcpkg" (
        echo [INFO] Cloning vcpkg...
        git clone https://github.com/microsoft/vcpkg.git
    )
    echo [INFO] Bootstrapping vcpkg...
    cd vcpkg
    call bootstrap-vcpkg.bat
    cd ..
)

:: 3. Install Libraries
echo.
echo [3/5] Installing Libraries (Raylib, ImGui)...
call "vcpkg\vcpkg.exe" install raylib rlimgui imgui[core,docking-experimental,win32-binding,glfw-binding,opengl3-binding] --triplet=x64-windows

:: 4. Configure CMake
echo.
echo [4/5] Configuring Project...
if exist "build" (
    echo [INFO] Cleaning old build...
    rmdir /s /q build
)
mkdir build
cd build

cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows

:: 5. Build
echo.
echo [5/5] Compiling SED LAB...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo [FAIL] Build failed.
    cd ..
    pause
    exit /b 1
)

echo.
echo ========================================================
echo   INSTALLATION SUCCESSFUL!
echo ========================================================
cd ..

:: Create Launcher
echo @echo off > start_simulation.bat
echo cd build\Release >> start_simulation.bat
echo if exist sed_lab_final.exe ( >> start_simulation.bat
echo    sed_lab_final.exe >> start_simulation.bat
echo ) else ( >> start_simulation.bat
echo    echo Executable not found. Did build succeed? >> start_simulation.bat
echo    pause >> start_simulation.bat
echo ) >> start_simulation.bat

echo [INFO] Launcher created: start_simulation.bat
echo Run start_simulation.bat to launch.
pause

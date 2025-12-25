@echo off
setlocal EnableDelayedExpansion

echo ========================================================
echo   SED LAB - ULTIMATE INSTALLER (Windows)
echo   "Eternity is not just a concept, it compiles."
echo ========================================================
echo.

:: 1. Check for Dependencies (Git, CMake, VS Build Tools)
echo [1/5] Checking System Prerequisites...
where git >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Git is not found! Please install Git for Windows.
    echo         https://git-scm.com/download/win
    pause
    exit /b 1
) else (
    echo [OK] Git found.
)

where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake is not found! Please install CMake.
    echo         https://cmake.org/download/
    pause
    exit /b 1
) else (
    echo [OK] CMake found.
)

:: Check for cl.exe (Visual Studio Compiler)
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo [WARN] Visual Studio Compiler (cl.exe) not in PATH.
    echo        Attempting to create Visual Studio environment...
    
    :: Try standard locations for VsDevCmd.bat
    set "VS_DEV_CMD="
    
    :: VS 2022
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
        set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" (
        set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" (
        set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
    )
    
    :: VS 2019
    if "!VS_DEV_CMD!"=="" (
        if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" (
            set "VS_DEV_CMD=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
        )
    )

    if "!VS_DEV_CMD!"=="" (
         echo [ERROR] Could not auto-detect Visual Studio. Please open this script from "Developer Command Prompt for VS".
         pause
         exit /b 1
    ) else (
         echo [INFO] Found VS Dev Cmd at: "!VS_DEV_CMD!"
         call "!VS_DEV_CMD!" -arch=x64
    )
)

:: 2. Setup VCPKG
echo.
echo [2/5] Setting up Dependency Manager (vcpkg)...
if not exist "vcpkg" (
    echo [INFO] Cloning vcpkg...
    git clone https://github.com/microsoft/vcpkg.git
)

if not exist "vcpkg\vcpkg.exe" (
    echo [INFO] Bootstrapping vcpkg...
    cd vcpkg
    call bootstrap-vcpkg.bat
    cd ..
)
echo [OK] vcpkg ready.

:: 3. Install Libraries
echo.
echo [3/5] Installing Libraries (Raylib, ImGui)...
echo [INFO] This might take a while on first run...
.\vcpkg\vcpkg.exe install raylib rlimgui imgui[core,docking-experimental,win32-binding,glfw-binding,opengl3-binding] --triplet=x64-windows

:: 4. Configure CMake
echo.
echo [4/5] Configuring Project...
if not exist "build" mkdir build
cd build

cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows

:: 5. Build
echo.
echo [5/5] Compiling SED LAB...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo [FAIL] Build failed. check errors above.
    cd ..
    pause
    exit /b 1
)

echo.
echo ========================================================
echo   INSTALLATION SUCCESSFUL!
echo   Run "start_simulation.bat" to launch.
echo ========================================================
cd ..

:: Create Launcher
echo @echo off > start_simulation.bat
echo cd build\Release >> start_simulation.bat
echo if exist sed_lab.exe ( >> start_simulation.bat
echo    sed_lab.exe >> start_simulation.bat
echo ) else ( >> start_simulation.bat
echo    echo Executable not found. Did build succeed? >> start_simulation.bat
echo    pause >> start_simulation.bat
echo ) >> start_simulation.bat

echo [INFO] Launcher created: start_simulation.bat
pause

@echo off
setlocal
set "LOGFILE=%~dp0launcher_log.txt"

echo [BATCH LAUNCHER STARTING] > "%LOGFILE%"
echo [%DATE% %TIME%] Starting processing... >> "%LOGFILE%"

:: ---------------------------------------------------------
:: 1. Admin Rights Check
:: ---------------------------------------------------------
echo Checking permissions...
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [INFO] Running with Administrator privileges.
    goto :check_dependencies
) else (
    echo [WARN] Not running as Administrator. Requesting elevation...
    echo [%DATE% %TIME%] Requesting elevation. >> "%LOGFILE%"
    :: Use PowerShell to restart script as Admin. 
    :: This bypasses execution policy by not running a .ps1 file directly.
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

:check_dependencies
:: ---------------------------------------------------------
:: 2. Dependencies Checks & Installs
:: ---------------------------------------------------------
echo.
echo === Checking Dependencies ===

:: Git
where git >nul 2>&1
if %errorLevel% neq 0 (
    echo [MISSING] Git not found. Installing via Winget...
    echo [%DATE% %TIME%] Installing Git... >> "%LOGFILE%"
    winget install -e --id Git.Git --accept-source-agreements --accept-package-agreements
    if %errorLevel% neq 0 (
        echo [ERROR] Failed to install Git. check logs.
        echo [%DATE% %TIME%] Failed to install Git. >> "%LOGFILE%"
        goto :error
    )
    echo [INFO] Git installed.
    :: Refresh env vars is hard in batch, we might need a restart or explicit path usage.
    :: For now, we continue and hope the path update propagates or user restarts.
) else (
    echo [OK] Git found.
)

:: CMake
where cmake >nul 2>&1
if %errorLevel% neq 0 (
    echo [MISSING] CMake not found. Installing via Winget...
    echo [%DATE% %TIME%] Installing CMake... >> "%LOGFILE%"
    winget install -e --id Kitware.CMake --accept-source-agreements --accept-package-agreements
    if %errorLevel% neq 0 (
        echo [ERROR] Failed to install CMake.
        echo [%DATE% %TIME%] Failed to install CMake. >> "%LOGFILE%"
        goto :error
    )
    echo [INFO] CMake installed.
) else (
    echo [OK] CMake found.
)

:: VS Build Tools (Generic check for 'cl.exe' or similar)
:: It's hard to check for VS easily in batch without massive scripts.
:: We will trust the user or the build failure if missing.
:: However, we can use vswhere if present.
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 >nul 2>&1
    if %errorLevel% neq 0 (
        echo [MISSING] Visual Studio C++ Tools not found. Attempting install...
        echo [%DATE% %TIME%] Installing VS Build Tools... >> "%LOGFILE%"
        winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
    ) else (
         echo [OK] Visual Studio C++ Tools detected.
    )
) else (
    echo [WARN] vswhere not found. Skipping VS Check. If build fails, install Visual Studio C++ Desktop workload manually.
)


:: ---------------------------------------------------------
:: 3. Vcpkg Setup
:: ---------------------------------------------------------
echo.
echo === Checking Libraries (vcpkg) ===
set "VCPKG_ROOT=%~dp0vcpkg"

if not exist "%VCPKG_ROOT%" (
    echo [INFO] Cloning vcpkg...
    git clone https://github.com/Microsoft/vcpkg.git "%VCPKG_ROOT%"
)

if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo [INFO] Bootstrapping vcpkg...
    cd /d "%VCPKG_ROOT%"
    call bootstrap-vcpkg.bat
    cd /d "%~dp0"
)

echo [INFO] Installing Raylib...
"%VCPKG_ROOT%\vcpkg.exe" install raylib:x64-windows
if %errorLevel% neq 0 (
    echo [ERROR] Failed to install Raylib using vcpkg.
    echo [%DATE% %TIME%] Failed to install Raylib. >> "%LOGFILE%"
    goto :error
)


:: ---------------------------------------------------------
:: 4. Build Process
:: ---------------------------------------------------------
echo.
echo === Building Project ===
:: CLEAN CLEANUP to prevent compiler mismatch issues (MinGW vs MSVC)
if exist "%~dp0build" (
    echo [INFO] Cleaning previous build...
    rmdir /s /q "%~dp0build"
)
mkdir "%~dp0build"
cd /d "%~dp0build"

echo [INFO] Configuring CMake (Forcing Visual Studio 2022)...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"
if %errorLevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    echo [%DATE% %TIME%] CMake config failed. >> "%LOGFILE%"
    goto :error
)

echo [INFO] Compiling (Release config)...
cmake --build . --config Release
if %errorLevel% neq 0 (
    echo [ERROR] Compilation failed.
    echo [%DATE% %TIME%] Compilation failed. >> "%LOGFILE%"
    goto :error
)


:: ---------------------------------------------------------
:: 5. Launch
:: ---------------------------------------------------------
echo.
echo === Launching Application ===
set "EXE_PATH_DEBUG=%~dp0build\Debug\sed_lab.exe"
set "EXE_PATH_RELEASE=%~dp0build\Release\sed_lab.exe"
set "EXE_PATH_ROOT=%~dp0build\sed_lab.exe"

if exist "%EXE_PATH_DEBUG%" (
    set "EXE_PATH=%EXE_PATH_DEBUG%"
) else if exist "%EXE_PATH_ROOT%" (
    set "EXE_PATH=%EXE_PATH_ROOT%"
) else if exist "%EXE_PATH_RELEASE%" (
    set "EXE_PATH=%EXE_PATH_RELEASE%"
) else (
    echo [ERROR] Executable not found. Checked:
    echo   - %EXE_PATH_DEBUG%
    echo   - %EXE_PATH_ROOT%
    echo   - %EXE_PATH_RELEASE%
    echo [%DATE% %TIME%] Exe not found. >> "%LOGFILE%"
    goto :error
)

echo [SUCCESS] Launching %EXE_PATH%...
echo [%DATE% %TIME%] Launching %EXE_PATH% >> "%LOGFILE%"

:: Run the executable synchronously to capture output
echo Running application...
"%EXE_PATH%"

if %errorLevel% neq 0 (
    echo.
    echo [ERROR] Application crashed or exited with error code %errorLevel%.
    echo [%DATE% %TIME%] Application crashed. Exit code: %errorLevel% >> "%LOGFILE%"
) else (
    echo.
    echo [INFO] Application exited normally.
)

echo.
echo Press any key to close this launcher...
pause
exit /b %errorLevel%

:error
echo.
echo [FATAL ERROR] An error occurred. Please check launcher_log.txt.
echo Press any key to exit...
pause
exit /b 1

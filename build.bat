@echo off
echo Building Backup and Recovery System on Windows...
echo.

REM Check if build directory exists
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

cd build

REM Try vcpkg first if available
if exist "C:\vcpkg\scripts\buildsystems\vcpkg.cmake" (
    echo Using vcpkg toolchain at C:\vcpkg...
    cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -A x64
) else if exist "C:\tools\vcpkg\scripts\buildsystems\vcpkg.cmake" (
    echo Using vcpkg toolchain at C:\tools\vcpkg...
    cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\tools\vcpkg\scripts\buildsystems\vcpkg.cmake -A x64
) else (
    echo No vcpkg found, using system libraries...
    echo Note: If you have vcpkg installed elsewhere, please update this script
    cmake .. -A x64
)

REM Build the project
if %errorlevel% == 0 (
    echo.
    echo Configuring successful, building...
    cmake --build . --config Release
    
    if %errorlevel% == 0 (
        echo.
        echo ======================================
        echo BUILD SUCCESSFUL!
        echo ======================================
        echo.
        echo Executable location: build\Release\backup_system.exe
        echo.
        echo To test the application:
        echo   .\Release\backup_system.exe --help
        echo.
        echo To run a simple backup:
        echo   .\Release\backup_system.exe --backup --source C:\path\to\source --dest C:\path\to\backup
        echo.
    ) else (
        echo.
        echo ======================================
        echo BUILD FAILED!
        echo ======================================
        echo Please check the error messages above.
        echo.
        echo Common solutions:
        echo 1. Install Visual Studio with C++ development tools
        echo 2. Install dependencies using vcpkg:
        echo    vcpkg install zlib openssl nlohmann-json
        echo 3. Check DEPENDENCIES.md for detailed instructions
    )
) else (
    echo.
    echo ======================================
    echo CONFIGURATION FAILED!
    echo ======================================
    echo Please check that all dependencies are installed.
    echo See DEPENDENCIES.md for installation instructions.
)

cd ..
pause

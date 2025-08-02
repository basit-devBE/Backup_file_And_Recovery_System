@echo off
echo.
echo ==============================================
echo   Backup and Recovery System - Windows Demo
echo ==============================================
echo.

REM Check if the executable exists
if not exist "build\Release\backup_system.exe" (
    echo ERROR: backup_system.exe not found!
    echo Please run build.bat first to build the project.
    pause
    exit /b 1
)

REM Clean up any previous demo files
if exist "demo_test" rmdir /s /q demo_test
if exist "demo_backup" rmdir /s /q demo_backup
if exist "demo_restore" rmdir /s /q demo_restore

echo Step 1: Creating test data...
mkdir demo_test\documents
mkdir demo_test\code

echo This is an important document. > demo_test\documents\important.txt
echo Welcome to the backup system! > demo_test\documents\readme.txt
echo #include ^<iostream^> > demo_test\code\hello.cpp
echo int main() { return 0; } >> demo_test\code\hello.cpp

echo    Created test files:
dir /s demo_test

echo.
echo Step 2: Creating backup...
build\Release\backup_system.exe --backup --source demo_test --dest demo_backup

echo.
echo Step 3: Listing backups...
build\Release\backup_system.exe --list --dest demo_backup

echo.
echo Step 4: Verifying backup integrity...
for /f "tokens=*" %%i in ('dir /b demo_backup') do (
    build\Release\backup_system.exe --verify --backup-path demo_backup\%%i
    goto :found_backup
)
:found_backup

echo.
echo Step 5: Restoring backup...
for /f "tokens=*" %%i in ('dir /b demo_backup') do (
    build\Release\backup_system.exe --restore --backup-path demo_backup\%%i --restore-path demo_restore
    goto :restore_done
)
:restore_done

echo.
echo Step 6: Comparing original vs restored...
echo Original files:
dir /s demo_test
echo.
echo Restored files:
dir /s demo_restore

echo.
echo ======================================
echo   Demo completed successfully!
echo ======================================
echo.
echo Key Features Demonstrated:
echo   - Full backup creation
echo   - Backup verification  
echo   - File restoration
echo   - Directory structure preservation
echo.
echo To test with compression:
echo   build\Release\backup_system.exe --backup --source demo_test --dest demo_backup_compressed --compress
echo.
echo Clean up: rmdir /s /q demo_test demo_backup demo_restore
echo.
pause

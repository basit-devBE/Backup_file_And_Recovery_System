<#
.SYNOPSIS
    Automatic dependency installer for Backup and Recovery System on Windows 11
    
.DESCRIPTION
    This script automatically installs all required dependencies including:
    - Visual Studio Build Tools
    - CMake
    - Git
    - vcpkg package manager
    - ZLIB, OpenSSL, and nlohmann-json libraries
    
.NOTES
    Run this script in PowerShell as Administrator
    Windows 11 compatible
#>

param(
    [switch]$SkipVisualStudio,
    [switch]$UseChocolatey,
    [string]$VcpkgPath = "C:\vcpkg"
)

# Set error handling
$ErrorActionPreference = "Stop"

# Colors for output
$Red = "Red"
$Green = "Green"
$Yellow = "Yellow"
$Blue = "Cyan"

function Write-ColorOutput($ForegroundColor) {
    $fc = $host.UI.RawUI.ForegroundColor
    $host.UI.RawUI.ForegroundColor = $ForegroundColor
    if ($args) {
        Write-Output $args
    }
    $host.UI.RawUI.ForegroundColor = $fc
}

function Write-Success($message) { Write-ColorOutput $Green $message }
function Write-Warning($message) { Write-ColorOutput $Yellow $message }
function Write-Error($message) { Write-ColorOutput $Red $message }
function Write-Info($message) { Write-ColorOutput $Blue $message }

function Test-Administrator {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentUser)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Install-Chocolatey {
    Write-Info "Installing Chocolatey package manager..."
    try {
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
        Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
        Write-Success "✓ Chocolatey installed successfully"
        
        # Refresh environment
        $env:PATH = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
        
        return $true
    }
    catch {
        Write-Error "✗ Failed to install Chocolatey: $($_.Exception.Message)"
        return $false
    }
}

function Install-VisualStudioBuildTools {
    Write-Info "Installing Visual Studio Build Tools 2022..."
    try {
        if (Get-Command "choco" -ErrorAction SilentlyContinue) {
            choco install visualstudio2022buildtools --package-parameters "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --includeOptional --passive" -y
        } else {
            # Download and install manually
            $vsUrl = "https://aka.ms/vs/17/release/vs_buildtools.exe"
            $vsInstaller = "$env:TEMP\vs_buildtools.exe"
            
            Write-Info "Downloading Visual Studio Build Tools..."
            Invoke-WebRequest -Uri $vsUrl -OutFile $vsInstaller
            
            Write-Info "Installing Visual Studio Build Tools (this may take a while)..."
            Start-Process -FilePath $vsInstaller -ArgumentList "--add", "Microsoft.VisualStudio.Workload.VCTools", "--includeRecommended", "--includeOptional", "--passive", "--wait" -Wait
            
            Remove-Item $vsInstaller -Force
        }
        Write-Success "✓ Visual Studio Build Tools installed successfully"
        return $true
    }
    catch {
        Write-Error "✗ Failed to install Visual Studio Build Tools: $($_.Exception.Message)"
        return $false
    }
}

function Install-BasicTools {
    Write-Info "Installing basic development tools..."
    try {
        if (Get-Command "choco" -ErrorAction SilentlyContinue) {
            Write-Info "Installing Git..."
            choco install git -y
            
            Write-Info "Installing CMake..."
            choco install cmake -y
            
            Write-Info "Installing 7-Zip (for extraction utilities)..."
            choco install 7zip -y
        } else {
            Write-Warning "Chocolatey not available. Please install Git and CMake manually:"
            Write-Info "Git: https://git-scm.com/download/win"
            Write-Info "CMake: https://cmake.org/download/"
        }
        
        # Refresh environment
        $env:PATH = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
        
        Write-Success "✓ Basic tools installation completed"
        return $true
    }
    catch {
        Write-Error "✗ Failed to install basic tools: $($_.Exception.Message)"
        return $false
    }
}

function Install-Vcpkg {
    param([string]$InstallPath = "C:\vcpkg")
    
    Write-Info "Installing vcpkg package manager..."
    try {
        if (Test-Path $InstallPath) {
            Write-Warning "vcpkg already exists at $InstallPath. Updating..."
            Set-Location $InstallPath
            git pull
        } else {
            Write-Info "Cloning vcpkg to $InstallPath..."
            git clone https://github.com/Microsoft/vcpkg.git $InstallPath
            Set-Location $InstallPath
        }
        
        Write-Info "Bootstrapping vcpkg..."
        .\bootstrap-vcpkg.bat
        
        Write-Info "Integrating vcpkg with Visual Studio..."
        .\vcpkg integrate install
        
        # Add vcpkg to PATH
        $currentPath = [System.Environment]::GetEnvironmentVariable("Path", "User")
        if ($currentPath -notlike "*$InstallPath*") {
            [System.Environment]::SetEnvironmentVariable("Path", $currentPath + ";$InstallPath", "User")
            $env:PATH += ";$InstallPath"
        }
        
        Write-Success "✓ vcpkg installed and configured successfully"
        return $InstallPath
    }
    catch {
        Write-Error "✗ Failed to install vcpkg: $($_.Exception.Message)"
        return $null
    }
}

function Install-ProjectDependencies {
    param([string]$VcpkgPath)
    
    Write-Info "Installing project dependencies via vcpkg..."
    try {
        Set-Location $VcpkgPath
        
        $packages = @(
            "zlib:x64-windows",
            "openssl:x64-windows", 
            "nlohmann-json:x64-windows"
        )
        
        foreach ($package in $packages) {
            Write-Info "Installing $package..."
            .\vcpkg install $package
            if ($LASTEXITCODE -ne 0) {
                throw "Failed to install $package"
            }
            Write-Success "✓ $package installed successfully"
        }
        
        Write-Success "✓ All project dependencies installed successfully"
        return $true
    }
    catch {
        Write-Error "✗ Failed to install project dependencies: $($_.Exception.Message)"
        return $false
    }
}

function Test-Dependencies {
    Write-Info "Verifying installed dependencies..."
    
    $tools = @{
        "git" = "Git"
        "cmake" = "CMake"
    }
    
    $allGood = $true
    
    foreach ($tool in $tools.Keys) {
        try {
            $version = & $tool --version 2>$null
            if ($version) {
                Write-Success "✓ $($tools[$tool]) is available"
            } else {
                Write-Error "✗ $($tools[$tool]) not found"
                $allGood = $false
            }
        }
        catch {
            Write-Error "✗ $($tools[$tool]) not found"
            $allGood = $false
        }
    }
    
    # Check vcpkg libraries
    if (Test-Path "$VcpkgPath\installed\x64-windows") {
        $libraries = @("zlib", "openssl", "nlohmann-json")
        foreach ($lib in $libraries) {
            if (Test-Path "$VcpkgPath\installed\x64-windows\include") {
                Write-Success "✓ $lib library available"
            } else {
                Write-Warning "? $lib library may not be properly installed"
            }
        }
    }
    
    return $allGood
}

function Show-BuildInstructions {
    param([string]$VcpkgPath)
    
    Write-Success "`n🎉 Installation completed successfully!"
    Write-Info "`n📋 Next Steps:"
    Write-Info "1. Open a new PowerShell or Command Prompt (to refresh environment variables)"
    Write-Info "2. Navigate to your project directory"
    Write-Info "3. Build the project using one of these methods:"
    Write-Info ""
    Write-Info "Method 1 - Using the batch file:"
    Write-ColorOutput $Yellow "   .\build.bat"
    Write-Info ""
    Write-Info "Method 2 - Manual CMake with vcpkg:"
    Write-ColorOutput $Yellow "   mkdir build"
    Write-ColorOutput $Yellow "   cd build"
    Write-ColorOutput $Yellow "   cmake .. -DCMAKE_TOOLCHAIN_FILE=$VcpkgPath\scripts\buildsystems\vcpkg.cmake"
    Write-ColorOutput $Yellow "   cmake --build . --config Release"
    Write-Info ""
    Write-Info "Method 3 - Open in Visual Studio:"
    Write-ColorOutput $Yellow "   # Open the generated .sln file in Visual Studio"
    Write-Info ""
    Write-Success "🚀 Your backup system will be built and ready to use!"
}

# Main execution
Write-Info "==================================================================="
Write-Info "    Backup and Recovery System - Windows 11 Dependency Installer"
Write-Info "==================================================================="
Write-Info ""

# Check if running as administrator
if (-not (Test-Administrator)) {
    Write-Error "❌ This script must be run as Administrator!"
    Write-Info "Right-click PowerShell and select 'Run as Administrator'"
    exit 1
}

Write-Info "✅ Running as Administrator"
Write-Info ""

# Set execution policy
try {
    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser -Force
    Write-Success "✓ PowerShell execution policy updated"
}
catch {
    Write-Warning "⚠ Could not update execution policy: $($_.Exception.Message)"
}

$success = $true

# Install Chocolatey if requested or if easier
if ($UseChocolatey -or -not (Get-Command "winget" -ErrorAction SilentlyContinue)) {
    if (-not (Get-Command "choco" -ErrorAction SilentlyContinue)) {
        $success = $success -and (Install-Chocolatey)
    } else {
        Write-Success "✓ Chocolatey already installed"
    }
}

# Install Visual Studio Build Tools
if (-not $SkipVisualStudio) {
    $success = $success -and (Install-VisualStudioBuildTools)
} else {
    Write-Warning "⚠ Skipping Visual Studio Build Tools installation"
}

# Install basic tools
$success = $success -and (Install-BasicTools)

# Install vcpkg
$vcpkgInstallPath = Install-Vcpkg -InstallPath $VcpkgPath
if ($vcpkgInstallPath) {
    Write-Success "✓ vcpkg installed at $vcpkgInstallPath"
    
    # Install project dependencies
    $success = $success -and (Install-ProjectDependencies -VcpkgPath $vcpkgInstallPath)
} else {
    $success = $false
}

# Verify installation
if ($success) {
    Write-Info ""
    $verification = Test-Dependencies
    
    if ($verification) {
        Show-BuildInstructions -VcpkgPath $vcpkgInstallPath
    } else {
        Write-Warning "⚠ Some dependencies may not be properly installed. Check the output above."
    }
} else {
    Write-Error "❌ Installation failed. Please check the errors above and try again."
    exit 1
}

Write-Info ""
Write-Info "==================================================================="
Write-Info "Installation script completed. Check output above for any issues."
Write-Info "==================================================================="

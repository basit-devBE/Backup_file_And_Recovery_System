<#
.SYNOPSIS
    Simple dependency installer for Backup and Recovery System (Windows 11)
    
.DESCRIPTION
    Simplified version that installs only the essential dependencies
#>

# Check administrator privileges
if (-not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "❌ Please run this script as Administrator!" -ForegroundColor Red
    exit 1
}

Write-Host "🚀 Installing dependencies for Backup and Recovery System..." -ForegroundColor Green

# Install Chocolatey
Write-Host "📦 Installing Chocolatey..." -ForegroundColor Cyan
Set-ExecutionPolicy Bypass -Scope Process -Force
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# Refresh PATH
$env:PATH = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

# Install basic tools
Write-Host "🔧 Installing development tools..." -ForegroundColor Cyan
choco install git cmake visualstudio2022buildtools -y

# Install vcpkg and libraries
Write-Host "📚 Installing vcpkg and libraries..." -ForegroundColor Cyan
cd C:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Install project dependencies
Write-Host "⚙️ Installing project libraries..." -ForegroundColor Cyan
.\vcpkg install zlib:x64-windows openssl:x64-windows nlohmann-json:x64-windows

Write-Host "✅ Installation completed!" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Open a new PowerShell/Command Prompt" -ForegroundColor White
Write-Host "2. Navigate to your project folder" -ForegroundColor White
Write-Host "3. Run: .\build.bat" -ForegroundColor White

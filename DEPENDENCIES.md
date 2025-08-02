# Dependencies Installation Guide

## Required Dependencies

This project requires the following libraries and tools to build successfully:

### 1. Build Tools
- **CMake** (version 3.12 or higher)
- **C++ Compiler** with C++17 support (GCC 7+ or Clang 5+)
- **Make**

### 2. Libraries

#### ZLIB (Compression)
```bash
# Ubuntu/Debian
sudo apt-get install zlib1g-dev

# CentOS/RHEL/Fedora
sudo yum install zlib-devel
# or
sudo dnf install zlib-devel

# macOS (with Homebrew)
brew install zlib
```

#### OpenSSL (Encryption)
```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# CentOS/RHEL/Fedora
sudo yum install openssl-devel
# or
sudo dnf install openssl-devel

# macOS (with Homebrew)
brew install openssl
```

#### nlohmann/json (JSON handling)
```bash
# Ubuntu 18.04+/Debian 10+
sudo apt-get install nlohmann-json3-dev

# For older systems or manual installation:
# Download single header file from: https://github.com/nlohmann/json/releases
# Place json.hpp in /usr/local/include/nlohmann/ or include/ directory

# Alternative: Use package manager
# vcpkg: vcpkg install nlohmann-json
# Conan: conan install nlohmann_json/3.11.2@
```

#### Threading (usually built-in)
- **pthread** (POSIX threads - usually available on Linux/macOS)

### 3. Installation Commands by Platform

#### Ubuntu/Debian (Complete)
```bash
sudo apt-get update
sudo apt-get install build-essential cmake zlib1g-dev libssl-dev nlohmann-json3-dev
```

#### CentOS/RHEL 8+
```bash
sudo dnf install gcc-c++ cmake zlib-devel openssl-devel
# For nlohmann-json, download manually or use EPEL
sudo dnf install epel-release
sudo dnf install json-devel
```

#### macOS
```bash
# Install Xcode command line tools
xcode-select --install

# Install dependencies with Homebrew
brew install cmake zlib openssl nlohmann-json
```

#### Windows

##### Option 1: Using vcpkg (Recommended)
```powershell
# Install vcpkg (in PowerShell as Administrator)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg install zlib:x64-windows
.\vcpkg install openssl:x64-windows
.\vcpkg install nlohmann-json:x64-windows

# Integrate with Visual Studio
.\vcpkg integrate install
```

##### Option 2: Using Chocolatey
```powershell
# Install Chocolatey first (in PowerShell as Administrator)
Set-ExecutionPolicy Bypass -Scope Process -Force
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# Install dependencies
choco install cmake
choco install git
choco install visualstudio2022buildtools
```

##### Option 3: Manual Installation
1. **Install Visual Studio 2019/2022** with C++ development tools
2. **Download and install CMake** from https://cmake.org/download/
3. **Download libraries manually:**
   - ZLIB: https://www.zlib.net/
   - OpenSSL: https://slproweb.com/products/Win32OpenSSL.html
   - nlohmann/json: https://github.com/nlohmann/json/releases

##### Option 4: Using MSYS2/MinGW-w64
```bash
# Install MSYS2 from https://www.msys2.org/
# Then in MSYS2 terminal:
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-zlib
pacman -S mingw-w64-x86_64-openssl
pacman -S mingw-w64-x86_64-nlohmann-json
```

### 4. Manual nlohmann/json Installation

If your system doesn't have nlohmann/json package:

```bash
# Download the single header file
curl -L https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp -o json.hpp

# Create directory and copy header
sudo mkdir -p /usr/local/include/nlohmann
sudo cp json.hpp /usr/local/include/nlohmann/

# Or place in project include directory
mkdir -p include/nlohmann
cp json.hpp include/nlohmann/
```

### 5. Build Verification

After installing dependencies, verify they're available:

```bash
# Check CMake
cmake --version

# Check compiler
g++ --version

# Check libraries (should not error)
pkg-config --libs zlib
pkg-config --libs openssl
```

### 6. Build the Project

#### Linux/macOS
```bash
# Method 1: Using build script
./build.sh

# Method 2: Manual CMake
mkdir build && cd build
cmake ..
make

# Method 3: Using Makefile (fallback)
make
```

#### Windows

##### With vcpkg (Visual Studio)
```powershell
# Create build directory
mkdir build
cd build

# Configure with vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build with Visual Studio
cmake --build . --config Release

# Or open the generated .sln file in Visual Studio
```

##### With MSYS2/MinGW-w64
```bash
# In MSYS2 terminal
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

##### With Visual Studio (manual dependencies)
```powershell
# Set environment variables for libraries
$env:ZLIB_ROOT = "C:\path\to\zlib"
$env:OPENSSL_ROOT_DIR = "C:\path\to\openssl"

# Configure and build
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

## Troubleshooting

### Common Issues:

#### All Platforms:
1. **"nlohmann/json.hpp not found"**
   - Install nlohmann-json3-dev package or download header manually

2. **"zlib.h not found"**
   - Install zlib1g-dev (Ubuntu) or zlib-devel (CentOS/RHEL)

3. **"openssl/evp.h not found"**
   - Install libssl-dev (Ubuntu) or openssl-devel (CentOS/RHEL)

4. **CMake version too old**
   - Update CMake or use the Makefile fallback

5. **C++17 not supported**
   - Update your compiler (GCC 7+ or Clang 5+)

#### Windows-Specific Issues:

6. **"Cannot find vcpkg.cmake"**
   - Ensure vcpkg is properly installed and path is correct
   - Use absolute path: `-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`

7. **"MSBuild not found" or "Visual Studio not detected"**
   - Install Visual Studio 2019/2022 with C++ development workload
   - Or install Visual Studio Build Tools

8. **"Permission denied" during vcpkg installation**
   - Run PowerShell as Administrator
   - Check Windows Defender or antivirus blocking

9. **Libraries not found with manual installation**
   - Set environment variables:
     ```powershell
     $env:ZLIB_ROOT = "C:\path\to\zlib"
     $env:OPENSSL_ROOT_DIR = "C:\path\to\openssl"
     ```

10. **"Execution policy" error in PowerShell**
    - Run: `Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser`

### Running the Application

#### Linux/macOS:
```bash
# After building
./build/backup_system --help

# Run demo
./simple_demo.sh
```

#### Windows:
```powershell
# After building (in build directory)
.\Release\backup_system.exe --help

# Or from project root
.\build\Release\backup_system.exe --help

# For demo script, use Git Bash or WSL, or create equivalent .bat file
```

### Cross-Platform Notes:
- **Path separators**: The application handles both `/` and `\` automatically
- **File permissions**: Windows may require "Run as Administrator" for some operations
- **Long paths**: Windows users may need to enable long path support in registry
- **Antivirus**: Some antivirus software may flag the backup executable

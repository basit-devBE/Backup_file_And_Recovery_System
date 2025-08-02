cmake_minimum_required(VERSION 3.12)
project(BackupSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/BackupManager.cpp
    src/FileTracker.cpp
    src/Compressor.cpp
    src/Encryptor.cpp
    src/Scheduler.cpp
    src/BackupMetadata.cpp
    src/Utils.cpp
)

# Create executable
add_executable(backup_system ${SOURCES})

# Link libraries
target_link_libraries(backup_system 
    ZLIB::ZLIB 
    OpenSSL::SSL 
    OpenSSL::Crypto 
    Threads::Threads
)

# Compiler flags
target_compile_options(backup_system PRIVATE
    -Wall -Wextra -O2
)

# Install target
install(TARGETS backup_system DESTINATION bin)

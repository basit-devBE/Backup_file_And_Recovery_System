# Backup and Recovery System

A comprehensive backup system implemented in C++ with incremental backups, compression, encryption, restoration, verification, and scheduling capabilities.

## Features

- **Incremental Backups**: Only backup changed files since last backup
- **Compression**: ZLIB compression to reduce backup size
- **Encryption**: AES encryption for secure backups
- **Restoration**: Full and selective file restoration
- **Verification**: Backup integrity checking
- **Scheduling**: Automated backup scheduling
- **Cross-platform**: Works on Linux, Windows, and macOS

## Key Components

1. **BackupManager**: Main class coordinating backup operations
2. **FileTracker**: Tracks file changes for incremental backups
3. **Compressor**: Handles file compression/decompression
4. **Encryptor**: Manages encryption/decryption
5. **Scheduler**: Handles automated backup scheduling
6. **BackupMetadata**: Stores backup information and checksums

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

```bash
# Create a full backup
./backup_system --backup --source /path/to/source --dest /path/to/backup

# Create incremental backup
./backup_system --incremental --source /path/to/source --dest /path/to/backup

# Restore from backup
./backup_system --restore --backup /path/to/backup --dest /path/to/restore

# Verify backup integrity
./backup_system --verify --backup /path/to/backup

# Schedule automatic backups
./backup_system --schedule --source /path/to/source --dest /path/to/backup --interval 3600
```

## Learning Goals Covered

- **File Systems**: File operations, directory traversal, metadata handling
- **Backup Strategies**: Full vs incremental backups, deduplication
- **Compression**: ZLIB compression algorithms
- **Data Integrity**: Checksums, verification, error detection

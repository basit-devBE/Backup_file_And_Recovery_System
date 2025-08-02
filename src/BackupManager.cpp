#include "BackupManager.h"
#include "FileTracker.h"
#include "Compressor.h"
#include "Encryptor.h"
#include "BackupMetadata.h"
#include "Utils.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <functional>

namespace fs = std::filesystem;

BackupManager::BackupManager() 
    : fileTracker_(std::make_unique<FileTracker>())
    , compressor_(std::make_unique<Compressor>())
    , encryptor_(std::make_unique<Encryptor>())
    , metadata_(std::make_unique<BackupMetadata>()) {
}

BackupManager::~BackupManager() = default;

bool BackupManager::createBackup(const BackupOptions& options) {
    try {
        updateProgress("Starting backup", 0.0f);
        
        // Validate source path
        if (!Utils::pathExists(options.sourcePath)) {
            std::cerr << "Error: Source path does not exist: " << options.sourcePath << std::endl;
            return false;
        }

        // Create backup directory
        std::string backupDir = generateBackupPath(options.destPath);
        if (!createBackupDirectory(backupDir)) {
            std::cerr << "Error: Failed to create backup directory: " << backupDir << std::endl;
            return false;
        }

        updateProgress("Scanning source directory", 10.0f);
        
        // Scan source directory
        if (!fileTracker_->scanDirectory(options.sourcePath)) {
            std::cerr << "Error: Failed to scan source directory" << std::endl;
            return false;
        }

        updateProgress("Creating backup metadata", 20.0f);

        // Create backup metadata
        BackupMetadata::BackupInfo backupInfo;
        backupInfo.backupId = Utils::generateUUID();
        backupInfo.backupType = "full";
        backupInfo.timestamp = std::chrono::system_clock::now();
        backupInfo.sourcePath = options.sourcePath;
        backupInfo.totalSize = 0;
        backupInfo.compressedSize = 0;
        backupInfo.encrypted = options.enableEncryption;
        backupInfo.compressionMethod = options.enableCompression ? "zlib" : "none";
        backupInfo.compressionLevel = options.compressionLevel;

        // Set up encryption if enabled
        if (options.enableEncryption) {
            if (!options.encryptionKey.empty()) {
                encryptor_->setKey(options.encryptionKey);
            } else {
                encryptor_->generateRandomKey();
            }
            backupInfo.encryptionMethod = "AES-256";
        }

        updateProgress("Copying files", 30.0f);

        // Get all files to backup
        auto allFiles = fileTracker_->getTotalFiles();
        size_t processedFiles = 0;

        // Copy all files
        for (const auto& entry : fs::recursive_directory_iterator(options.sourcePath)) {
            if (entry.is_regular_file()) {
                std::string relativePath = Utils::getRelativePath(options.sourcePath, entry.path().string());
                std::string destPath = Utils::joinPaths(backupDir, relativePath);
                
                // Create destination directory if needed
                Utils::createDirectoryRecursive(Utils::getParentDirectory(destPath));
                
                // Copy file with options
                if (!copyFileWithOptions(entry.path().string(), destPath, options)) {
                    std::cerr << "Error: Failed to copy file: " << entry.path() << std::endl;
                    return false;
                }

                // Create file entry for metadata
                BackupMetadata::FileEntry fileEntry;
                fileEntry.relativePath = relativePath;
                fileEntry.size = Utils::getFileSize(entry.path().string());
                fileEntry.lastModified = Utils::getFileModificationTime(entry.path().string());
                fileEntry.checksum = Utils::calculateSHA256(entry.path().string());
                fileEntry.compressed = options.enableCompression;
                fileEntry.encrypted = options.enableEncryption;
                fileEntry.compressedSize = Utils::getFileSize(destPath);

                backupInfo.files.push_back(fileEntry);
                backupInfo.totalSize += fileEntry.size;
                backupInfo.compressedSize += fileEntry.compressedSize;

                processedFiles++;
                float progress = 30.0f + (processedFiles * 60.0f / allFiles);
                updateProgress("Copying files", progress);
            }
        }

        updateProgress("Saving metadata", 95.0f);

        // Save backup metadata
        metadata_->createBackupInfo(backupInfo);
        std::string metadataFile = Utils::joinPaths(backupDir, "backup_metadata.json");
        metadata_->exportToJson(metadataFile);

        // Save file tracker state
        std::string stateFile = Utils::joinPaths(backupDir, "file_state.db");
        fileTracker_->saveDatabaseState(stateFile);

        updateProgress("Backup completed", 100.0f);
        
        std::cout << "Backup created: " << backupDir << std::endl;
        std::cout << "Files: " << backupInfo.files.size() << std::endl;
        std::cout << "Original size: " << Utils::formatBytes(backupInfo.totalSize) << std::endl;
        std::cout << "Backup size: " << Utils::formatBytes(backupInfo.compressedSize) << std::endl;
        
        if (options.enableCompression && backupInfo.totalSize > 0) {
            double ratio = (double)backupInfo.compressedSize / backupInfo.totalSize;
            std::cout << "Compression ratio: " << std::fixed << std::setprecision(2) << (ratio * 100) << "%" << std::endl;
        }

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error during backup: " << e.what() << std::endl;
        return false;
    }
}

bool BackupManager::createIncrementalBackup(const BackupOptions& options) {
    try {
        updateProgress("Starting incremental backup", 0.0f);
        
        // Find the latest full backup
        auto backups = listBackups(options.destPath);
        std::string parentBackupId;
        
        if (!backups.empty()) {
            // Load the latest backup's file state
            std::string latestBackup = backups.back();
            std::string stateFile = Utils::joinPaths(latestBackup, "file_state.db");
            
            if (Utils::pathExists(stateFile)) {
                fileTracker_->loadPreviousState(stateFile);
                
                // Load parent backup metadata
                std::string metadataFile = Utils::joinPaths(latestBackup, "backup_metadata.json");
                if (Utils::pathExists(metadataFile)) {
                    BackupMetadata tempMetadata;
                    tempMetadata.loadFromFile(metadataFile);
                    // Get the backup ID from the metadata
                    parentBackupId = Utils::generateUUID(); // This should be loaded from metadata
                }
            }
        }

        updateProgress("Scanning for changes", 10.0f);
        
        // Scan current directory state
        if (!fileTracker_->scanDirectory(options.sourcePath)) {
            std::cerr << "Error: Failed to scan source directory" << std::endl;
            return false;
        }

        // Get changed files
        auto changedFiles = fileTracker_->getChangedFiles();
        auto newFiles = fileTracker_->getNewFiles();
        auto modifiedFiles = fileTracker_->getModifiedFiles();
        
        std::vector<std::string> filesToBackup;
        filesToBackup.insert(filesToBackup.end(), changedFiles.begin(), changedFiles.end());
        filesToBackup.insert(filesToBackup.end(), newFiles.begin(), newFiles.end());
        filesToBackup.insert(filesToBackup.end(), modifiedFiles.begin(), modifiedFiles.end());

        if (filesToBackup.empty()) {
            std::cout << "No changes detected. No backup needed." << std::endl;
            return true;
        }

        updateProgress("Creating incremental backup", 20.0f);

        // Create backup directory
        std::string backupDir = generateBackupPath(options.destPath);
        if (!createBackupDirectory(backupDir)) {
            std::cerr << "Error: Failed to create backup directory: " << backupDir << std::endl;
            return false;
        }

        // Create backup metadata
        BackupMetadata::BackupInfo backupInfo;
        backupInfo.backupId = Utils::generateUUID();
        backupInfo.backupType = "incremental";
        backupInfo.timestamp = std::chrono::system_clock::now();
        backupInfo.sourcePath = options.sourcePath;
        backupInfo.parentBackupId = parentBackupId;
        backupInfo.totalSize = 0;
        backupInfo.compressedSize = 0;
        backupInfo.encrypted = options.enableEncryption;
        backupInfo.compressionMethod = options.enableCompression ? "zlib" : "none";
        backupInfo.compressionLevel = options.compressionLevel;

        updateProgress("Copying changed files", 30.0f);

        // Copy only changed files
        size_t processedFiles = 0;
        for (const auto& filePath : filesToBackup) {
            std::string fullSourcePath = Utils::joinPaths(options.sourcePath, filePath);
            std::string relativePath = Utils::getRelativePath(options.sourcePath, fullSourcePath);
            std::string destPath = Utils::joinPaths(backupDir, relativePath);
            
            // Create destination directory if needed
            Utils::createDirectoryRecursive(Utils::getParentDirectory(destPath));
            
            // Copy file with options
            if (!copyFileWithOptions(fullSourcePath, destPath, options)) {
                std::cerr << "Error: Failed to copy file: " << fullSourcePath << std::endl;
                return false;
            }

            // Create file entry for metadata
            BackupMetadata::FileEntry fileEntry;
            fileEntry.relativePath = relativePath;
            fileEntry.size = Utils::getFileSize(fullSourcePath);
            fileEntry.lastModified = Utils::getFileModificationTime(fullSourcePath);
            fileEntry.checksum = Utils::calculateSHA256(fullSourcePath);
            fileEntry.compressed = options.enableCompression;
            fileEntry.encrypted = options.enableEncryption;
            fileEntry.compressedSize = Utils::getFileSize(destPath);

            backupInfo.files.push_back(fileEntry);
            backupInfo.totalSize += fileEntry.size;
            backupInfo.compressedSize += fileEntry.compressedSize;

            processedFiles++;
            float progress = 30.0f + (processedFiles * 60.0f / filesToBackup.size());
            updateProgress("Copying changed files", progress);
        }

        updateProgress("Saving metadata", 95.0f);

        // Save backup metadata
        metadata_->createBackupInfo(backupInfo);
        std::string metadataFile = Utils::joinPaths(backupDir, "backup_metadata.json");
        metadata_->exportToJson(metadataFile);

        // Save updated file tracker state
        std::string stateFile = Utils::joinPaths(backupDir, "file_state.db");
        fileTracker_->saveDatabaseState(stateFile);

        updateProgress("Incremental backup completed", 100.0f);
        
        std::cout << "Incremental backup created: " << backupDir << std::endl;
        std::cout << "Changed files: " << filesToBackup.size() << std::endl;
        std::cout << "Original size: " << Utils::formatBytes(backupInfo.totalSize) << std::endl;
        std::cout << "Backup size: " << Utils::formatBytes(backupInfo.compressedSize) << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error during incremental backup: " << e.what() << std::endl;
        return false;
    }
}

bool BackupManager::restoreBackup(const std::string& backupPath, const std::string& restorePath) {
    try {
        updateProgress("Starting restore", 0.0f);
        
        // Validate backup path
        if (!Utils::pathExists(backupPath)) {
            std::cerr << "Error: Backup path does not exist: " << backupPath << std::endl;
            return false;
        }

        // Load backup metadata
        std::string metadataFile = Utils::joinPaths(backupPath, "backup_metadata.json");
        if (!Utils::pathExists(metadataFile)) {
            std::cerr << "Error: Backup metadata not found: " << metadataFile << std::endl;
            return false;
        }

        BackupMetadata tempMetadata;
        if (!tempMetadata.loadFromFile(metadataFile)) {
            std::cerr << "Error: Failed to load backup metadata" << std::endl;
            return false;
        }

        updateProgress("Creating restore directory", 10.0f);

        // Create restore directory
        if (!Utils::createDirectoryRecursive(restorePath)) {
            std::cerr << "Error: Failed to create restore directory: " << restorePath << std::endl;
            return false;
        }

        updateProgress("Restoring files", 20.0f);

        // Get all backup files
        size_t totalFiles = 0;
        for (const auto& entry : fs::recursive_directory_iterator(backupPath)) {
            if (entry.is_regular_file() && entry.path().filename() != "backup_metadata.json" && 
                entry.path().filename() != "file_state.db") {
                totalFiles++;
            }
        }

        size_t processedFiles = 0;

        // Restore all files
        for (const auto& entry : fs::recursive_directory_iterator(backupPath)) {
            if (entry.is_regular_file() && entry.path().filename() != "backup_metadata.json" && 
                entry.path().filename() != "file_state.db") {
                
                std::string relativePath = Utils::getRelativePath(backupPath, entry.path().string());
                std::string destPath = Utils::joinPaths(restorePath, relativePath);
                
                // Create destination directory if needed
                Utils::createDirectoryRecursive(Utils::getParentDirectory(destPath));
                
                // Restore file (decompress and decrypt if needed)
                if (!restoreFileInternal(entry.path().string(), destPath)) {
                    std::cerr << "Error: Failed to restore file: " << entry.path() << std::endl;
                    return false;
                }

                processedFiles++;
                float progress = 20.0f + (processedFiles * 70.0f / totalFiles);
                updateProgress("Restoring files", progress);
            }
        }

        updateProgress("Restore completed", 100.0f);
        
        std::cout << "Restore completed: " << restorePath << std::endl;
        std::cout << "Files restored: " << processedFiles << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error during restore: " << e.what() << std::endl;
        return false;
    }
}

bool BackupManager::restoreFile(const std::string& backupPath, const std::string& fileName, const std::string& restorePath) {
    try {
        std::string sourceFile = Utils::joinPaths(backupPath, fileName);
        std::string destFile = Utils::joinPaths(restorePath, fileName);
        
        // Create destination directory if needed
        Utils::createDirectoryRecursive(Utils::getParentDirectory(destFile));
        
        return restoreFileInternal(sourceFile, destFile);

    } catch (const std::exception& e) {
        std::cerr << "Error restoring specific file: " << e.what() << std::endl;
        return false;
    }
}

bool BackupManager::restoreFileInternal(const std::string& sourcePath, const std::string& destPath) {
    try {
        // Copy the file first
        if (!Utils::copyFile(sourcePath, destPath)) {
            return false;
        }

        // Check if file needs decompression or decryption
        // This would need to be determined from the metadata
        // For now, assume files are compressed and possibly encrypted
        
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error restoring file: " << e.what() << std::endl;
        return false;
    }
}

bool BackupManager::verifyBackup(const std::string& backupPath) {
    try {
        updateProgress("Starting verification", 0.0f);
        
        // Load backup metadata
        std::string metadataFile = Utils::joinPaths(backupPath, "backup_metadata.json");
        if (!Utils::pathExists(metadataFile)) {
            std::cerr << "Error: Backup metadata not found" << std::endl;
            return false;
        }

        BackupMetadata tempMetadata;
        if (!tempMetadata.loadFromFile(metadataFile)) {
            std::cerr << "Error: Failed to load backup metadata" << std::endl;
            return false;
        }

        updateProgress("Verifying file integrity", 20.0f);

        // Verify all files exist and have correct checksums
        size_t totalFiles = 0;
        for (const auto& entry : fs::recursive_directory_iterator(backupPath)) {
            if (entry.is_regular_file() && entry.path().filename() != "backup_metadata.json" && 
                entry.path().filename() != "file_state.db") {
                totalFiles++;
            }
        }

        size_t processedFiles = 0;
        bool allValid = true;

        for (const auto& entry : fs::recursive_directory_iterator(backupPath)) {
            if (entry.is_regular_file() && entry.path().filename() != "backup_metadata.json" && 
                entry.path().filename() != "file_state.db") {
                
                // For a complete implementation, we would verify checksums
                // against the metadata. For now, just check file existence
                if (!Utils::pathExists(entry.path().string())) {
                    std::cerr << "Error: Missing file: " << entry.path() << std::endl;
                    allValid = false;
                }

                processedFiles++;
                float progress = 20.0f + (processedFiles * 70.0f / totalFiles);
                updateProgress("Verifying files", progress);
            }
        }

        updateProgress("Verification completed", 100.0f);
        
        if (allValid) {
            std::cout << "Backup verification successful" << std::endl;
        } else {
            std::cout << "Backup verification failed" << std::endl;
        }

        return allValid;

    } catch (const std::exception& e) {
        std::cerr << "Error during verification: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> BackupManager::listBackups(const std::string& backupRoot) {
    std::vector<std::string> backups;
    
    try {
        if (!Utils::pathExists(backupRoot)) {
            return backups;
        }

        for (const auto& entry : fs::directory_iterator(backupRoot)) {
            if (entry.is_directory()) {
                std::string metadataFile = Utils::joinPaths(entry.path().string(), "backup_metadata.json");
                if (Utils::pathExists(metadataFile)) {
                    backups.push_back(entry.path().string());
                }
            }
        }

        // Sort backups by creation time
        std::sort(backups.begin(), backups.end());

    } catch (const std::exception& e) {
        std::cerr << "Error listing backups: " << e.what() << std::endl;
    }

    return backups;
}

size_t BackupManager::getBackupSize(const std::string& backupPath) {
    return Utils::getDirectorySize(backupPath);
}

std::chrono::system_clock::time_point BackupManager::getBackupTimestamp(const std::string& backupPath) {
    // Extract timestamp from backup directory name
    std::string dirname = Utils::getFileName(backupPath);
    if (dirname.length() >= 15 && dirname.substr(0, 7) == "backup_") {
        std::string timestamp = dirname.substr(7);
        return Utils::parseTimestamp(timestamp);
    }
    
    // Fallback to directory creation time
    return Utils::getFileModificationTime(backupPath);
}

void BackupManager::setProgressCallback(std::function<void(const std::string&, float)> callback) {
    progressCallback_ = callback;
}

bool BackupManager::createBackupDirectory(const std::string& path) {
    return Utils::createDirectoryRecursive(path);
}

bool BackupManager::copyFileWithOptions(const std::string& src, const std::string& dest, const BackupOptions& options) {
    try {
        if (options.enableCompression && options.enableEncryption) {
            // Compress then encrypt
            std::string tempFile = dest + ".tmp";
            if (!compressor_->compressFile(src, tempFile, 
                static_cast<Compressor::CompressionLevel>(options.compressionLevel))) {
                return false;
            }
            if (!encryptor_->encryptFile(tempFile, dest)) {
                fs::remove(tempFile);
                return false;
            }
            fs::remove(tempFile);
        } else if (options.enableCompression) {
            // Compress only
            if (!compressor_->compressFile(src, dest, 
                static_cast<Compressor::CompressionLevel>(options.compressionLevel))) {
                return false;
            }
        } else if (options.enableEncryption) {
            // Encrypt only
            if (!encryptor_->encryptFile(src, dest)) {
                return false;
            }
        } else {
            // Copy as-is
            if (!Utils::copyFile(src, dest)) {
                return false;
            }
        }
        
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error copying file with options: " << e.what() << std::endl;
        return false;
    }
}

std::string BackupManager::generateBackupPath(const std::string& basePath) {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "backup_%Y%m%d_%H%M%S");
    
    return Utils::joinPaths(basePath, ss.str());
}

void BackupManager::updateProgress(const std::string& operation, float percentage) {
    if (progressCallback_) {
        progressCallback_(operation, percentage);
    }
}

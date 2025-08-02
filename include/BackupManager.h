#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>

class FileTracker;
class Compressor;
class Encryptor;
class BackupMetadata;

/**
 * Main backup manager that coordinates all backup operations
 */
class BackupManager {
public:
    struct BackupOptions {
        std::string sourcePath;
        std::string destPath;
        bool enableCompression = true;
        bool enableEncryption = false;
        std::string encryptionKey;
        bool incremental = false;
        int compressionLevel = 6;
    };

    BackupManager();
    ~BackupManager();

    // Core backup operations
    bool createBackup(const BackupOptions& options);
    bool createIncrementalBackup(const BackupOptions& options);
    bool restoreBackup(const std::string& backupPath, const std::string& restorePath);
    bool restoreFile(const std::string& backupPath, const std::string& fileName, const std::string& restorePath);
    
    // Verification and integrity
    bool verifyBackup(const std::string& backupPath);
    bool verifyFile(const std::string& filePath, const std::string& expectedChecksum);
    
    // Information and status
    std::vector<std::string> listBackups(const std::string& backupRoot);
    size_t getBackupSize(const std::string& backupPath);
    std::chrono::system_clock::time_point getBackupTimestamp(const std::string& backupPath);
    
    // Progress callback
    void setProgressCallback(std::function<void(const std::string&, float)> callback);

private:
    std::unique_ptr<FileTracker> fileTracker_;
    std::unique_ptr<Compressor> compressor_;
    std::unique_ptr<Encryptor> encryptor_;
    std::unique_ptr<BackupMetadata> metadata_;
    
    std::function<void(const std::string&, float)> progressCallback_;
    
    // Helper methods
    bool createBackupDirectory(const std::string& path);
    bool copyFileWithOptions(const std::string& src, const std::string& dest, const BackupOptions& options);
    bool restoreFileInternal(const std::string& src, const std::string& dest);
    std::string generateBackupPath(const std::string& basePath);
    void updateProgress(const std::string& operation, float percentage);
};

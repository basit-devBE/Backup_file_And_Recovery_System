#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <nlohmann/json.hpp>

/**
 * Manages backup metadata and information
 */
class BackupMetadata {
public:
    struct FileEntry {
        std::string relativePath;
        std::string checksum;
        std::uintmax_t size;
        std::chrono::system_clock::time_point lastModified;
        bool compressed;
        bool encrypted;
        std::uintmax_t compressedSize;
    };

    struct BackupInfo {
        std::string backupId;
        std::string backupType; // "full" or "incremental"
        std::chrono::system_clock::time_point timestamp;
        std::string sourcePath;
        std::string parentBackupId; // For incremental backups
        std::vector<FileEntry> files;
        std::uintmax_t totalSize;
        std::uintmax_t compressedSize;
        bool encrypted;
        std::string encryptionMethod;
        std::string compressionMethod;
        int compressionLevel;
    };

    BackupMetadata();
    ~BackupMetadata();

    // Backup information management
    bool createBackupInfo(const BackupInfo& info);
    bool updateBackupInfo(const std::string& backupId, const BackupInfo& info);
    BackupInfo getBackupInfo(const std::string& backupId) const;
    bool deleteBackupInfo(const std::string& backupId);
    
    // File entry management
    bool addFileEntry(const std::string& backupId, const FileEntry& entry);
    bool removeFileEntry(const std::string& backupId, const std::string& relativePath);
    std::vector<FileEntry> getFileEntries(const std::string& backupId) const;
    FileEntry getFileEntry(const std::string& backupId, const std::string& relativePath) const;
    
    // Backup chain management
    std::vector<std::string> getBackupChain(const std::string& backupId) const;
    std::string getFullBackupId(const std::string& incrementalBackupId) const;
    std::vector<std::string> getIncrementalBackups(const std::string& fullBackupId) const;
    
    // Verification and integrity
    bool verifyBackupIntegrity(const std::string& backupId) const;
    std::string calculateBackupChecksum(const std::string& backupId) const;
    bool validateFileChecksums(const std::string& backupId) const;
    
    // Search and query
    std::vector<std::string> listAllBackups() const;
    std::vector<std::string> findBackupsContainingFile(const std::string& relativePath) const;
    std::vector<std::string> findBackupsByDateRange(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) const;
    
    // Statistics
    std::uintmax_t getTotalBackupSize(const std::string& backupId) const;
    size_t getFileCount(const std::string& backupId) const;
    double getCompressionRatio(const std::string& backupId) const;
    
    // Persistence
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);
    bool exportToJson(const std::string& filename) const;
    bool importFromJson(const std::string& filename);
    
    // Cleanup
    bool cleanupOrphanedEntries();
    bool removeOldBackups(const std::chrono::system_clock::time_point& cutoffDate);

private:
    std::unordered_map<std::string, BackupInfo> backups_;
    
    // Helper methods
    std::string generateBackupId() const;
    nlohmann::json backupInfoToJson(const BackupInfo& info) const;
    BackupInfo backupInfoFromJson(const nlohmann::json& json) const;
    nlohmann::json fileEntryToJson(const FileEntry& entry) const;
    FileEntry fileEntryFromJson(const nlohmann::json& json) const;
    bool validateBackupInfo(const BackupInfo& info) const;
};

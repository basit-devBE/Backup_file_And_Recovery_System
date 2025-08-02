#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <filesystem>

/**
 * Tracks file changes to enable incremental backups
 */
class FileTracker {
public:
    struct FileInfo {
        std::string path;
        std::uintmax_t size;
        std::chrono::system_clock::time_point lastModified;
        std::string checksum;
        bool isDirectory;
    };

    FileTracker();
    ~FileTracker();

    // File tracking operations
    bool scanDirectory(const std::string& path);
    bool loadPreviousState(const std::string& stateFile);
    bool saveDatabaseState(const std::string& stateFile);
    
    // Change detection
    std::vector<std::string> getChangedFiles();
    std::vector<std::string> getNewFiles();
    std::vector<std::string> getDeletedFiles();
    std::vector<std::string> getModifiedFiles();
    
    // File information
    bool hasFileChanged(const std::string& filePath);
    FileInfo getFileInfo(const std::string& filePath);
    std::string calculateFileChecksum(const std::string& filePath);
    
    // Database management
    void updateFileInfo(const std::string& filePath, const FileInfo& info);
    void removeFile(const std::string& filePath);
    void clear();
    
    // Statistics
    size_t getTotalFiles() const;
    size_t getChangedFilesCount() const;
    size_t getTotalSize() const;

private:
    std::unordered_map<std::string, FileInfo> currentState_;
    std::unordered_map<std::string, FileInfo> previousState_;
    
    // Helper methods
    FileInfo createFileInfo(const std::filesystem::directory_entry& entry);
    std::string calculateChecksumSHA256(const std::string& filePath);
    bool compareFileInfo(const FileInfo& current, const FileInfo& previous) const;
    void scanDirectoryRecursive(const std::string& path);
};

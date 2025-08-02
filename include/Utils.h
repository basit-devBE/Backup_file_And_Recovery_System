#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <filesystem>

/**
 * Utility functions for the backup system
 */
class Utils {
public:
    // File system utilities
    static bool createDirectoryRecursive(const std::string& path);
    static bool deleteDirectoryRecursive(const std::string& path);
    static bool copyFile(const std::string& source, const std::string& dest);
    static bool moveFile(const std::string& source, const std::string& dest);
    static std::uintmax_t getFileSize(const std::string& filePath);
    static std::uintmax_t getDirectorySize(const std::string& dirPath);
    
    // Path utilities
    static std::string getRelativePath(const std::string& basePath, const std::string& fullPath);
    static std::string joinPaths(const std::string& path1, const std::string& path2);
    static std::string getParentDirectory(const std::string& path);
    static std::string getFileName(const std::string& path);
    static std::string getFileExtension(const std::string& path);
    static bool pathExists(const std::string& path);
    static bool isDirectory(const std::string& path);
    static bool isRegularFile(const std::string& path);
    
    // String utilities
    static std::string formatBytes(std::uintmax_t bytes);
    static std::string formatDuration(std::chrono::seconds duration);
    static std::string formatTimestamp(const std::chrono::system_clock::time_point& timePoint);
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static std::string trim(const std::string& str);
    static std::string toLower(const std::string& str);
    static std::string toUpper(const std::string& str);
    
    // Checksum utilities
    static std::string calculateSHA256(const std::string& filePath);
    static std::string calculateSHA256(const std::vector<uint8_t>& data);
    static std::string calculateMD5(const std::string& filePath);
    static bool verifyChecksum(const std::string& filePath, const std::string& expectedChecksum);
    
    // Time utilities
    static std::chrono::system_clock::time_point parseTimestamp(const std::string& timestamp);
    static std::string timestampToString(const std::chrono::system_clock::time_point& timePoint);
    static std::chrono::system_clock::time_point getFileModificationTime(const std::string& filePath);
    
    // Random utilities
    static std::string generateRandomString(size_t length);
    static std::string generateUUID();
    static std::vector<uint8_t> generateRandomBytes(size_t length);
    
    // Validation utilities
    static bool isValidPath(const std::string& path);
    static bool isValidFileName(const std::string& filename);
    static bool hasReadPermission(const std::string& path);
    static bool hasWritePermission(const std::string& path);
    
    // System utilities
    static std::string getHomeDirectory();
    static std::string getTempDirectory();
    static std::uintmax_t getAvailableDiskSpace(const std::string& path);
    static std::uintmax_t getTotalDiskSpace(const std::string& path);
    
    // Error handling
    static std::string getLastErrorMessage();
    static void logError(const std::string& message);
    static void logInfo(const std::string& message);
    static void logWarning(const std::string& message);

private:
    Utils() = delete; // Static class
};

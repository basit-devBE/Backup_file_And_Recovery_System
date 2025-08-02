#include "BackupMetadata.h"
#include "Utils.h"
#include <fstream>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

BackupMetadata::BackupMetadata() = default;

BackupMetadata::~BackupMetadata() = default;

bool BackupMetadata::createBackupInfo(const BackupInfo& info) {
    if (!validateBackupInfo(info)) {
        return false;
    }
    
    backups_[info.backupId] = info;
    return true;
}

bool BackupMetadata::updateBackupInfo(const std::string& backupId, const BackupInfo& info) {
    auto it = backups_.find(backupId);
    if (it == backups_.end()) {
        return false;
    }
    
    if (!validateBackupInfo(info)) {
        return false;
    }
    
    backups_[backupId] = info;
    return true;
}

BackupMetadata::BackupInfo BackupMetadata::getBackupInfo(const std::string& backupId) const {
    auto it = backups_.find(backupId);
    if (it != backups_.end()) {
        return it->second;
    }
    
    // Return empty BackupInfo if not found
    BackupInfo empty;
    empty.backupId = "";
    return empty;
}

bool BackupMetadata::deleteBackupInfo(const std::string& backupId) {
    auto it = backups_.find(backupId);
    if (it != backups_.end()) {
        backups_.erase(it);
        return true;
    }
    return false;
}

bool BackupMetadata::addFileEntry(const std::string& backupId, const FileEntry& entry) {
    auto it = backups_.find(backupId);
    if (it == backups_.end()) {
        return false;
    }
    
    it->second.files.push_back(entry);
    return true;
}

bool BackupMetadata::removeFileEntry(const std::string& backupId, const std::string& relativePath) {
    auto it = backups_.find(backupId);
    if (it == backups_.end()) {
        return false;
    }
    
    auto& files = it->second.files;
    auto fileIt = std::find_if(files.begin(), files.end(),
                              [&relativePath](const FileEntry& entry) {
                                  return entry.relativePath == relativePath;
                              });
    
    if (fileIt != files.end()) {
        files.erase(fileIt);
        return true;
    }
    
    return false;
}

std::vector<BackupMetadata::FileEntry> BackupMetadata::getFileEntries(const std::string& backupId) const {
    auto it = backups_.find(backupId);
    if (it != backups_.end()) {
        return it->second.files;
    }
    return std::vector<FileEntry>();
}

BackupMetadata::FileEntry BackupMetadata::getFileEntry(const std::string& backupId, const std::string& relativePath) const {
    auto it = backups_.find(backupId);
    if (it != backups_.end()) {
        const auto& files = it->second.files;
        auto fileIt = std::find_if(files.begin(), files.end(),
                                  [&relativePath](const FileEntry& entry) {
                                      return entry.relativePath == relativePath;
                                  });
        
        if (fileIt != files.end()) {
            return *fileIt;
        }
    }
    
    // Return empty FileEntry if not found
    FileEntry empty;
    empty.relativePath = relativePath;
    return empty;
}

std::vector<std::string> BackupMetadata::getBackupChain(const std::string& backupId) const {
    std::vector<std::string> chain;
    std::string currentId = backupId;
    
    // Build chain from current backup back to the root full backup
    while (!currentId.empty()) {
        auto it = backups_.find(currentId);
        if (it == backups_.end()) {
            break;
        }
        
        chain.push_back(currentId);
        
        // Move to parent backup
        currentId = it->second.parentBackupId;
        
        // Prevent infinite loops
        if (std::find(chain.begin(), chain.end() - 1, currentId) != chain.end() - 1) {
            break;
        }
    }
    
    return chain;
}

std::string BackupMetadata::getFullBackupId(const std::string& incrementalBackupId) const {
    auto chain = getBackupChain(incrementalBackupId);
    
    // The full backup is the last one in the chain
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        auto backupIt = backups_.find(*it);
        if (backupIt != backups_.end() && backupIt->second.backupType == "full") {
            return *it;
        }
    }
    
    return "";
}

std::vector<std::string> BackupMetadata::getIncrementalBackups(const std::string& fullBackupId) const {
    std::vector<std::string> incrementals;
    
    for (const auto& pair : backups_) {
        const BackupInfo& info = pair.second;
        if (info.backupType == "incremental") {
            // Check if this incremental backup belongs to the chain
            std::string rootId = getFullBackupId(info.backupId);
            if (rootId == fullBackupId) {
                incrementals.push_back(info.backupId);
            }
        }
    }
    
    // Sort by timestamp
    std::sort(incrementals.begin(), incrementals.end(),
             [this](const std::string& a, const std::string& b) {
                 auto itA = backups_.find(a);
                 auto itB = backups_.find(b);
                 if (itA != backups_.end() && itB != backups_.end()) {
                     return itA->second.timestamp < itB->second.timestamp;
                 }
                 return false;
             });
    
    return incrementals;
}

bool BackupMetadata::verifyBackupIntegrity(const std::string& backupId) const {
    auto it = backups_.find(backupId);
    if (it == backups_.end()) {
        return false;
    }
    
    const BackupInfo& info = it->second;
    
    // Verify that all required fields are present
    if (info.backupId.empty() || info.backupType.empty() || info.sourcePath.empty()) {
        return false;
    }
    
    // Verify backup type
    if (info.backupType != "full" && info.backupType != "incremental") {
        return false;
    }
    
    // For incremental backups, verify parent exists
    if (info.backupType == "incremental") {
        if (info.parentBackupId.empty()) {
            return false;
        }
        
        auto parentIt = backups_.find(info.parentBackupId);
        if (parentIt == backups_.end()) {
            return false;
        }
    }
    
    // Verify file entries have required fields
    for (const auto& fileEntry : info.files) {
        if (fileEntry.relativePath.empty() || fileEntry.checksum.empty()) {
            return false;
        }
    }
    
    return true;
}

std::string BackupMetadata::calculateBackupChecksum(const std::string& backupId) const {
    auto it = backups_.find(backupId);
    if (it == backups_.end()) {
        return "";
    }
    
    // Create a string representation of the backup info for checksumming
    std::string data = it->second.backupId + it->second.backupType + it->second.sourcePath;
    
    for (const auto& fileEntry : it->second.files) {
        data += fileEntry.relativePath + fileEntry.checksum + std::to_string(fileEntry.size);
    }
    
    return Utils::calculateSHA256(std::vector<uint8_t>(data.begin(), data.end()));
}

bool BackupMetadata::validateFileChecksums(const std::string& backupId) const {
    // This would need access to the actual backup files to verify checksums
    // For now, just verify that checksums are present
    auto it = backups_.find(backupId);
    if (it == backups_.end()) {
        return false;
    }
    
    for (const auto& fileEntry : it->second.files) {
        if (fileEntry.checksum.empty()) {
            return false;
        }
    }
    
    return true;
}

std::vector<std::string> BackupMetadata::listAllBackups() const {
    std::vector<std::string> backupIds;
    
    for (const auto& pair : backups_) {
        backupIds.push_back(pair.first);
    }
    
    // Sort by timestamp
    std::sort(backupIds.begin(), backupIds.end(),
             [this](const std::string& a, const std::string& b) {
                 auto itA = backups_.find(a);
                 auto itB = backups_.find(b);
                 if (itA != backups_.end() && itB != backups_.end()) {
                     return itA->second.timestamp < itB->second.timestamp;
                 }
                 return false;
             });
    
    return backupIds;
}

std::vector<std::string> BackupMetadata::findBackupsContainingFile(const std::string& relativePath) const {
    std::vector<std::string> backupIds;
    
    for (const auto& pair : backups_) {
        const BackupInfo& info = pair.second;
        
        auto it = std::find_if(info.files.begin(), info.files.end(),
                              [&relativePath](const FileEntry& entry) {
                                  return entry.relativePath == relativePath;
                              });
        
        if (it != info.files.end()) {
            backupIds.push_back(pair.first);
        }
    }
    
    return backupIds;
}

std::vector<std::string> BackupMetadata::findBackupsByDateRange(
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end) const {
    
    std::vector<std::string> backupIds;
    
    for (const auto& pair : backups_) {
        const BackupInfo& info = pair.second;
        
        if (info.timestamp >= start && info.timestamp <= end) {
            backupIds.push_back(pair.first);
        }
    }
    
    return backupIds;
}

std::uintmax_t BackupMetadata::getTotalBackupSize(const std::string& backupId) const {
    auto it = backups_.find(backupId);
    if (it != backups_.end()) {
        return it->second.totalSize;
    }
    return 0;
}

size_t BackupMetadata::getFileCount(const std::string& backupId) const {
    auto it = backups_.find(backupId);
    if (it != backups_.end()) {
        return it->second.files.size();
    }
    return 0;
}

double BackupMetadata::getCompressionRatio(const std::string& backupId) const {
    auto it = backups_.find(backupId);
    if (it != backups_.end()) {
        const BackupInfo& info = it->second;
        if (info.totalSize > 0) {
            return static_cast<double>(info.compressedSize) / static_cast<double>(info.totalSize);
        }
    }
    return 0.0;
}

bool BackupMetadata::saveToFile(const std::string& filename) const {
    return exportToJson(filename);
}

bool BackupMetadata::loadFromFile(const std::string& filename) {
    return importFromJson(filename);
}

bool BackupMetadata::exportToJson(const std::string& filename) const {
    try {
        json j;
        j["version"] = "1.0";
        j["backups"] = json::array();
        
        for (const auto& pair : backups_) {
            j["backups"].push_back(backupInfoToJson(pair.second));
        }
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        file << j.dump(2);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error exporting metadata: " << e.what() << std::endl;
        return false;
    }
}

bool BackupMetadata::importFromJson(const std::string& filename) {
    try {
        if (!Utils::pathExists(filename)) {
            return true; // No metadata file is OK
        }
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        json j;
        file >> j;
        
        backups_.clear();
        
        for (const auto& backupJson : j["backups"]) {
            BackupInfo info = backupInfoFromJson(backupJson);
            if (!info.backupId.empty()) {
                backups_[info.backupId] = info;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error importing metadata: " << e.what() << std::endl;
        return false;
    }
}

bool BackupMetadata::cleanupOrphanedEntries() {
    // Remove incremental backups whose parent no longer exists
    std::vector<std::string> toRemove;
    
    for (const auto& pair : backups_) {
        const BackupInfo& info = pair.second;
        
        if (info.backupType == "incremental" && !info.parentBackupId.empty()) {
            if (backups_.find(info.parentBackupId) == backups_.end()) {
                toRemove.push_back(pair.first);
            }
        }
    }
    
    for (const std::string& backupId : toRemove) {
        backups_.erase(backupId);
    }
    
    return true;
}

bool BackupMetadata::removeOldBackups(const std::chrono::system_clock::time_point& cutoffDate) {
    std::vector<std::string> toRemove;
    
    for (const auto& pair : backups_) {
        const BackupInfo& info = pair.second;
        
        if (info.timestamp < cutoffDate) {
            toRemove.push_back(pair.first);
        }
    }
    
    for (const std::string& backupId : toRemove) {
        backups_.erase(backupId);
    }
    
    return true;
}

std::string BackupMetadata::generateBackupId() const {
    return Utils::generateUUID();
}

json BackupMetadata::backupInfoToJson(const BackupInfo& info) const {
    json j;
    j["backupId"] = info.backupId;
    j["backupType"] = info.backupType;
    j["timestamp"] = Utils::formatTimestamp(info.timestamp);
    j["sourcePath"] = info.sourcePath;
    j["parentBackupId"] = info.parentBackupId;
    j["totalSize"] = info.totalSize;
    j["compressedSize"] = info.compressedSize;
    j["encrypted"] = info.encrypted;
    j["encryptionMethod"] = info.encryptionMethod;
    j["compressionMethod"] = info.compressionMethod;
    j["compressionLevel"] = info.compressionLevel;
    
    j["files"] = json::array();
    for (const auto& fileEntry : info.files) {
        j["files"].push_back(fileEntryToJson(fileEntry));
    }
    
    return j;
}

BackupMetadata::BackupInfo BackupMetadata::backupInfoFromJson(const json& j) const {
    BackupInfo info;
    
    try {
        info.backupId = j["backupId"];
        info.backupType = j["backupType"];
        info.timestamp = Utils::parseTimestamp(j["timestamp"]);
        info.sourcePath = j["sourcePath"];
        info.parentBackupId = j.value("parentBackupId", "");
        info.totalSize = j["totalSize"];
        info.compressedSize = j["compressedSize"];
        info.encrypted = j["encrypted"];
        info.encryptionMethod = j.value("encryptionMethod", "");
        info.compressionMethod = j.value("compressionMethod", "");
        info.compressionLevel = j.value("compressionLevel", 6);
        
        for (const auto& fileJson : j["files"]) {
            info.files.push_back(fileEntryFromJson(fileJson));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing backup info from JSON: " << e.what() << std::endl;
        info.backupId = ""; // Mark as invalid
    }
    
    return info;
}

json BackupMetadata::fileEntryToJson(const FileEntry& entry) const {
    json j;
    j["relativePath"] = entry.relativePath;
    j["checksum"] = entry.checksum;
    j["size"] = entry.size;
    j["lastModified"] = Utils::formatTimestamp(entry.lastModified);
    j["compressed"] = entry.compressed;
    j["encrypted"] = entry.encrypted;
    j["compressedSize"] = entry.compressedSize;
    
    return j;
}

BackupMetadata::FileEntry BackupMetadata::fileEntryFromJson(const json& j) const {
    FileEntry entry;
    
    try {
        entry.relativePath = j["relativePath"];
        entry.checksum = j["checksum"];
        entry.size = j["size"];
        entry.lastModified = Utils::parseTimestamp(j["lastModified"]);
        entry.compressed = j["compressed"];
        entry.encrypted = j["encrypted"];
        entry.compressedSize = j["compressedSize"];
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing file entry from JSON: " << e.what() << std::endl;
    }
    
    return entry;
}

bool BackupMetadata::validateBackupInfo(const BackupInfo& info) const {
    if (info.backupId.empty() || info.backupType.empty() || info.sourcePath.empty()) {
        return false;
    }
    
    if (info.backupType != "full" && info.backupType != "incremental") {
        return false;
    }
    
    return true;
}

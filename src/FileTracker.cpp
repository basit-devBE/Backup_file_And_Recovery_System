#include "FileTracker.h"
#include "Utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

FileTracker::FileTracker() = default;

FileTracker::~FileTracker() = default;

bool FileTracker::scanDirectory(const std::string& path) {
    try {
        currentState_.clear();
        scanDirectoryRecursive(path);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
        return false;
    }
}

void FileTracker::scanDirectoryRecursive(const std::string& path) {
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        try {
            FileInfo info = createFileInfo(entry);
            currentState_[info.path] = info;
        } catch (const std::exception& e) {
            std::cerr << "Error processing file " << entry.path() << ": " << e.what() << std::endl;
        }
    }
}

FileTracker::FileInfo FileTracker::createFileInfo(const fs::directory_entry& entry) {
    FileInfo info;
    info.path = entry.path().string();
    info.isDirectory = entry.is_directory();
    
    if (!info.isDirectory) {
        info.size = entry.file_size();
        info.lastModified = Utils::getFileModificationTime(info.path);
        info.checksum = calculateFileChecksum(info.path);
    } else {
        info.size = 0;
        info.lastModified = Utils::getFileModificationTime(info.path);
        info.checksum = "";
    }
    
    return info;
}

bool FileTracker::loadPreviousState(const std::string& stateFile) {
    try {
        if (!Utils::pathExists(stateFile)) {
            return true; // No previous state is OK
        }
        
        std::ifstream file(stateFile);
        if (!file.is_open()) {
            return false;
        }
        
        json j;
        file >> j;
        
        previousState_.clear();
        
        for (const auto& item : j["files"]) {
            FileInfo info;
            info.path = item["path"];
            info.size = item["size"];
            info.isDirectory = item["isDirectory"];
            info.checksum = item["checksum"];
            
            // Parse timestamp
            std::string timestamp = item["lastModified"];
            info.lastModified = Utils::parseTimestamp(timestamp);
            
            previousState_[info.path] = info;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading previous state: " << e.what() << std::endl;
        return false;
    }
}

bool FileTracker::saveDatabaseState(const std::string& stateFile) {
    try {
        json j;
        j["version"] = "1.0";
        j["timestamp"] = Utils::formatTimestamp(std::chrono::system_clock::now());
        j["files"] = json::array();
        
        for (const auto& pair : currentState_) {
            const FileInfo& info = pair.second;
            json fileObj;
            fileObj["path"] = info.path;
            fileObj["size"] = info.size;
            fileObj["isDirectory"] = info.isDirectory;
            fileObj["checksum"] = info.checksum;
            fileObj["lastModified"] = Utils::formatTimestamp(info.lastModified);
            
            j["files"].push_back(fileObj);
        }
        
        std::ofstream file(stateFile);
        if (!file.is_open()) {
            return false;
        }
        
        file << j.dump(2);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving state: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> FileTracker::getChangedFiles() {
    std::vector<std::string> changed;
    
    // Find new and modified files
    for (const auto& pair : currentState_) {
        const std::string& path = pair.first;
        const FileInfo& currentInfo = pair.second;
        
        auto it = previousState_.find(path);
        if (it == previousState_.end()) {
            // New file
            changed.push_back(path);
        } else {
            // Check if modified
            const FileInfo& previousInfo = it->second;
            if (!compareFileInfo(currentInfo, previousInfo)) {
                changed.push_back(path);
            }
        }
    }
    
    return changed;
}

std::vector<std::string> FileTracker::getNewFiles() {
    std::vector<std::string> newFiles;
    
    for (const auto& pair : currentState_) {
        const std::string& path = pair.first;
        
        if (previousState_.find(path) == previousState_.end()) {
            newFiles.push_back(path);
        }
    }
    
    return newFiles;
}

std::vector<std::string> FileTracker::getDeletedFiles() {
    std::vector<std::string> deletedFiles;
    
    for (const auto& pair : previousState_) {
        const std::string& path = pair.first;
        
        if (currentState_.find(path) == currentState_.end()) {
            deletedFiles.push_back(path);
        }
    }
    
    return deletedFiles;
}

std::vector<std::string> FileTracker::getModifiedFiles() {
    std::vector<std::string> modifiedFiles;
    
    for (const auto& pair : currentState_) {
        const std::string& path = pair.first;
        const FileInfo& currentInfo = pair.second;
        
        auto it = previousState_.find(path);
        if (it != previousState_.end()) {
            const FileInfo& previousInfo = it->second;
            if (!compareFileInfo(currentInfo, previousInfo)) {
                modifiedFiles.push_back(path);
            }
        }
    }
    
    return modifiedFiles;
}

bool FileTracker::hasFileChanged(const std::string& filePath) {
    auto currentIt = currentState_.find(filePath);
    auto previousIt = previousState_.find(filePath);
    
    if (currentIt == currentState_.end()) {
        return previousIt != previousState_.end(); // File was deleted
    }
    
    if (previousIt == previousState_.end()) {
        return true; // New file
    }
    
    return !compareFileInfo(currentIt->second, previousIt->second);
}

FileTracker::FileInfo FileTracker::getFileInfo(const std::string& filePath) {
    auto it = currentState_.find(filePath);
    if (it != currentState_.end()) {
        return it->second;
    }
    
    // Return empty FileInfo if not found
    FileInfo empty;
    empty.path = filePath;
    empty.size = 0;
    empty.isDirectory = false;
    empty.lastModified = std::chrono::system_clock::now();
    empty.checksum = "";
    return empty;
}

std::string FileTracker::calculateFileChecksum(const std::string& filePath) {
    return Utils::calculateSHA256(filePath);
}

void FileTracker::updateFileInfo(const std::string& filePath, const FileInfo& info) {
    currentState_[filePath] = info;
}

void FileTracker::removeFile(const std::string& filePath) {
    currentState_.erase(filePath);
}

void FileTracker::clear() {
    currentState_.clear();
    previousState_.clear();
}

size_t FileTracker::getTotalFiles() const {
    return currentState_.size();
}

size_t FileTracker::getChangedFilesCount() const {
    size_t count = 0;
    
    for (const auto& pair : currentState_) {
        const std::string& path = pair.first;
        const FileInfo& currentInfo = pair.second;
        
        auto it = previousState_.find(path);
        if (it == previousState_.end()) {
            // New file
            count++;
        } else {
            // Check if modified
            const FileInfo& previousInfo = it->second;
            if (!compareFileInfo(currentInfo, previousInfo)) {
                count++;
            }
        }
    }
    
    return count;
}

size_t FileTracker::getTotalSize() const {
    size_t totalSize = 0;
    
    for (const auto& pair : currentState_) {
        if (!pair.second.isDirectory) {
            totalSize += pair.second.size;
        }
    }
    
    return totalSize;
}

bool FileTracker::compareFileInfo(const FileInfo& current, const FileInfo& previous) const {
    // Compare size first (fastest)
    if (current.size != previous.size) {
        return false;
    }
    
    // Compare modification time
    if (current.lastModified != previous.lastModified) {
        return false;
    }
    
    // For directories, size and time are sufficient
    if (current.isDirectory) {
        return true;
    }
    
    // For files, also compare checksum
    return current.checksum == previous.checksum;
}

std::string FileTracker::calculateChecksumSHA256(const std::string& filePath) {
    return Utils::calculateSHA256(filePath);
}

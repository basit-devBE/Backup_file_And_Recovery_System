#include "Utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <cstring>
#include <cerrno>
#include <openssl/sha.h>
#include <openssl/md5.h>

namespace fs = std::filesystem;

bool Utils::createDirectoryRecursive(const std::string& path) {
    try {
        // fs::create_directories returns false if directory already exists
        // We should return true if the directory exists or was created successfully
        fs::create_directories(path);
        return fs::exists(path) && fs::is_directory(path);
    } catch (const std::exception& e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
        return false;
    }
}

bool Utils::deleteDirectoryRecursive(const std::string& path) {
    try {
        return fs::remove_all(path) > 0;
    } catch (const std::exception& e) {
        std::cerr << "Error deleting directory: " << e.what() << std::endl;
        return false;
    }
}

bool Utils::copyFile(const std::string& source, const std::string& dest) {
    try {
        return fs::copy_file(source, dest, fs::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
        std::cerr << "Error copying file: " << e.what() << std::endl;
        return false;
    }
}

bool Utils::moveFile(const std::string& source, const std::string& dest) {
    try {
        fs::rename(source, dest);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error moving file: " << e.what() << std::endl;
        return false;
    }
}

std::uintmax_t Utils::getFileSize(const std::string& filePath) {
    try {
        return fs::file_size(filePath);
    } catch (const std::exception& e) {
        return 0;
    }
}

std::uintmax_t Utils::getDirectorySize(const std::string& dirPath) {
    std::uintmax_t size = 0;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(dirPath)) {
            if (entry.is_regular_file()) {
                size += entry.file_size();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error calculating directory size: " << e.what() << std::endl;
    }
    return size;
}

std::string Utils::getRelativePath(const std::string& basePath, const std::string& fullPath) {
    try {
        fs::path base(basePath);
        fs::path full(fullPath);
        return fs::relative(full, base).string();
    } catch (const std::exception& e) {
        // Fallback: simple string manipulation
        if (fullPath.find(basePath) == 0) {
            std::string relative = fullPath.substr(basePath.length());
            if (!relative.empty() && (relative[0] == '/' || relative[0] == '\\')) {
                relative = relative.substr(1);
            }
            return relative;
        }
        return fullPath;
    }
}

std::string Utils::joinPaths(const std::string& path1, const std::string& path2) {
    fs::path p1(path1);
    fs::path p2(path2);
    return (p1 / p2).string();
}

std::string Utils::getParentDirectory(const std::string& path) {
    return fs::path(path).parent_path().string();
}

std::string Utils::getFileName(const std::string& path) {
    return fs::path(path).filename().string();
}

std::string Utils::getFileExtension(const std::string& path) {
    return fs::path(path).extension().string();
}

bool Utils::pathExists(const std::string& path) {
    return fs::exists(path);
}

bool Utils::isDirectory(const std::string& path) {
    return fs::is_directory(path);
}

bool Utils::isRegularFile(const std::string& path) {
    return fs::is_regular_file(path);
}

std::string Utils::formatBytes(std::uintmax_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return ss.str();
}

std::string Utils::formatDuration(std::chrono::seconds duration) {
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1));
    auto seconds = duration % std::chrono::minutes(1);
    
    std::stringstream ss;
    if (hours.count() > 0) {
        ss << hours.count() << "h ";
    }
    if (minutes.count() > 0) {
        ss << minutes.count() << "m ";
    }
    ss << seconds.count() << "s";
    
    return ss.str();
}

std::string Utils::formatTimestamp(const std::chrono::system_clock::time_point& timePoint) {
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::vector<std::string> Utils::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string Utils::trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::string Utils::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string Utils::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::string Utils::calculateSHA256(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return "";
    }
    
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer))) {
        SHA256_Update(&sha256, buffer, file.gcount());
    }
    if (file.gcount() > 0) {
        SHA256_Update(&sha256, buffer, file.gcount());
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

std::string Utils::calculateSHA256(const std::vector<uint8_t>& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

std::string Utils::calculateMD5(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return "";
    }
    
    MD5_CTX md5;
    MD5_Init(&md5);
    
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer))) {
        MD5_Update(&md5, buffer, file.gcount());
    }
    if (file.gcount() > 0) {
        MD5_Update(&md5, buffer, file.gcount());
    }
    
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5_Final(hash, &md5);
    
    std::stringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

bool Utils::verifyChecksum(const std::string& filePath, const std::string& expectedChecksum) {
    std::string actualChecksum = calculateSHA256(filePath);
    return actualChecksum == expectedChecksum;
}

std::chrono::system_clock::time_point Utils::parseTimestamp(const std::string& timestamp) {
    std::tm tm = {};
    std::istringstream ss(timestamp);
    
    // Try to parse different formats
    if (timestamp.length() == 15) { // YYYYMMDD_HHMMSS
        ss >> std::get_time(&tm, "%Y%m%d_%H%M%S");
    } else {
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    }
    
    if (ss.fail()) {
        return std::chrono::system_clock::now();
    }
    
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string Utils::timestampToString(const std::chrono::system_clock::time_point& timePoint) {
    return formatTimestamp(timePoint);
}

std::chrono::system_clock::time_point Utils::getFileModificationTime(const std::string& filePath) {
    try {
        auto ftime = fs::last_write_time(filePath);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        return sctp;
    } catch (const std::exception& e) {
        return std::chrono::system_clock::now();
    }
}

std::string Utils::generateRandomString(size_t length) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    
    std::string result;
    result.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        result += chars[dis(gen)];
    }
    
    return result;
}

std::string Utils::generateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    ss << std::hex;
    
    for (int i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    
    for (int i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";
    
    for (int i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    
    for (int i = 0; i < 12; i++) {
        ss << dis(gen);
    }
    
    return ss.str();
}

std::vector<uint8_t> Utils::generateRandomBytes(size_t length) {
    std::vector<uint8_t> bytes(length);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    
    for (size_t i = 0; i < length; ++i) {
        bytes[i] = dis(gen);
    }
    
    return bytes;
}

bool Utils::isValidPath(const std::string& path) {
    try {
        fs::path p(path);
        return !p.empty() && p.is_absolute();
    } catch (const std::exception& e) {
        return false;
    }
}

bool Utils::isValidFileName(const std::string& filename) {
    if (filename.empty() || filename == "." || filename == "..") {
        return false;
    }
    
    // Check for invalid characters
    const std::string invalidChars = "<>:\"|?*";
    return filename.find_first_of(invalidChars) == std::string::npos;
}

bool Utils::hasReadPermission(const std::string& path) {
    try {
        auto perms = fs::status(path).permissions();
        return (perms & fs::perms::owner_read) != fs::perms::none;
    } catch (const std::exception& e) {
        return false;
    }
}

bool Utils::hasWritePermission(const std::string& path) {
    try {
        auto perms = fs::status(path).permissions();
        return (perms & fs::perms::owner_write) != fs::perms::none;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string Utils::getHomeDirectory() {
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home);
    }
    return "/tmp";
}

std::string Utils::getTempDirectory() {
    return fs::temp_directory_path().string();
}

std::uintmax_t Utils::getAvailableDiskSpace(const std::string& path) {
    try {
        auto space = fs::space(path);
        return space.available;
    } catch (const std::exception& e) {
        return 0;
    }
}

std::uintmax_t Utils::getTotalDiskSpace(const std::string& path) {
    try {
        auto space = fs::space(path);
        return space.capacity;
    } catch (const std::exception& e) {
        return 0;
    }
}

std::string Utils::getLastErrorMessage() {
    return std::strerror(errno);
}

void Utils::logError(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

void Utils::logInfo(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}

void Utils::logWarning(const std::string& message) {
    std::cout << "[WARNING] " << message << std::endl;
}

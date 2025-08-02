#pragma once

#include <string>
#include <vector>
#include <cstdint>

/**
 * Handles file compression and decompression using ZLIB
 */
class Compressor {
public:
    enum class CompressionLevel {
        NO_COMPRESSION = 0,
        BEST_SPEED = 1,
        BEST_COMPRESSION = 9,
        DEFAULT_COMPRESSION = 6
    };

    Compressor();
    ~Compressor();

    // File compression
    bool compressFile(const std::string& inputFile, const std::string& outputFile, 
                     CompressionLevel level = CompressionLevel::DEFAULT_COMPRESSION);
    bool decompressFile(const std::string& inputFile, const std::string& outputFile);
    
    // Memory compression
    std::vector<uint8_t> compressData(const std::vector<uint8_t>& data, 
                                     CompressionLevel level = CompressionLevel::DEFAULT_COMPRESSION);
    std::vector<uint8_t> decompressData(const std::vector<uint8_t>& compressedData);
    
    // String compression
    std::string compressString(const std::string& input, 
                              CompressionLevel level = CompressionLevel::DEFAULT_COMPRESSION);
    std::string decompressString(const std::string& compressed);
    
    // Utility functions
    double getCompressionRatio(const std::string& originalFile, const std::string& compressedFile);
    size_t getCompressedSize(const std::string& compressedFile);
    bool isCompressed(const std::string& filePath);
    
    // Statistics
    size_t getTotalBytesCompressed() const;
    size_t getTotalBytesOriginal() const;
    double getAverageCompressionRatio() const;

private:
    size_t totalBytesCompressed_;
    size_t totalBytesOriginal_;
    
    // Helper methods
    bool compressFileInternal(FILE* source, FILE* dest, int level);
    bool decompressFileInternal(FILE* source, FILE* dest);
    std::vector<uint8_t> processData(const std::vector<uint8_t>& data, bool compress, int level = 6);
};

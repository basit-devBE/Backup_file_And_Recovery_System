#include "Compressor.h"
#include <zlib.h>
#include <fstream>
#include <iostream>
#include <vector>

Compressor::Compressor() 
    : totalBytesCompressed_(0)
    , totalBytesOriginal_(0) {
}

Compressor::~Compressor() = default;

bool Compressor::compressFile(const std::string& inputFile, const std::string& outputFile, CompressionLevel level) {
    FILE* source = fopen(inputFile.c_str(), "rb");
    if (!source) {
        std::cerr << "Error: Cannot open input file: " << inputFile << std::endl;
        return false;
    }
    
    FILE* dest = fopen(outputFile.c_str(), "wb");
    if (!dest) {
        std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
        fclose(source);
        return false;
    }
    
    bool result = compressFileInternal(source, dest, static_cast<int>(level));
    
    fclose(source);
    fclose(dest);
    
    if (result) {
        // Update statistics
        std::ifstream in(inputFile, std::ios::binary | std::ios::ate);
        std::ifstream out(outputFile, std::ios::binary | std::ios::ate);
        
        if (in.is_open() && out.is_open()) {
            totalBytesOriginal_ += in.tellg();
            totalBytesCompressed_ += out.tellg();
        }
    }
    
    return result;
}

bool Compressor::decompressFile(const std::string& inputFile, const std::string& outputFile) {
    FILE* source = fopen(inputFile.c_str(), "rb");
    if (!source) {
        std::cerr << "Error: Cannot open compressed file: " << inputFile << std::endl;
        return false;
    }
    
    FILE* dest = fopen(outputFile.c_str(), "wb");
    if (!dest) {
        std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
        fclose(source);
        return false;
    }
    
    bool result = decompressFileInternal(source, dest);
    
    fclose(source);
    fclose(dest);
    
    return result;
}

std::vector<uint8_t> Compressor::compressData(const std::vector<uint8_t>& data, CompressionLevel level) {
    return processData(data, true, static_cast<int>(level));
}

std::vector<uint8_t> Compressor::decompressData(const std::vector<uint8_t>& compressedData) {
    return processData(compressedData, false);
}

std::string Compressor::compressString(const std::string& input, CompressionLevel level) {
    std::vector<uint8_t> inputData(input.begin(), input.end());
    std::vector<uint8_t> compressed = compressData(inputData, level);
    return std::string(compressed.begin(), compressed.end());
}

std::string Compressor::decompressString(const std::string& compressed) {
    std::vector<uint8_t> compressedData(compressed.begin(), compressed.end());
    std::vector<uint8_t> decompressed = decompressData(compressedData);
    return std::string(decompressed.begin(), decompressed.end());
}

double Compressor::getCompressionRatio(const std::string& originalFile, const std::string& compressedFile) {
    std::ifstream original(originalFile, std::ios::binary | std::ios::ate);
    std::ifstream compressed(compressedFile, std::ios::binary | std::ios::ate);
    
    if (!original.is_open() || !compressed.is_open()) {
        return 0.0;
    }
    
    size_t originalSize = original.tellg();
    size_t compressedSize = compressed.tellg();
    
    if (originalSize == 0) {
        return 0.0;
    }
    
    return static_cast<double>(compressedSize) / static_cast<double>(originalSize);
}

size_t Compressor::getCompressedSize(const std::string& compressedFile) {
    std::ifstream file(compressedFile, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return 0;
    }
    return file.tellg();
}

bool Compressor::isCompressed(const std::string& filePath) {
    // Check for zlib magic bytes (0x78 followed by various compression flags)
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    uint8_t header[2];
    file.read(reinterpret_cast<char*>(header), 2);
    
    if (file.gcount() < 2) {
        return false;
    }
    
    // Check for zlib header
    return header[0] == 0x78 && (header[1] == 0x01 || header[1] == 0x5E || 
                                header[1] == 0x9C || header[1] == 0xDA);
}

size_t Compressor::getTotalBytesCompressed() const {
    return totalBytesCompressed_;
}

size_t Compressor::getTotalBytesOriginal() const {
    return totalBytesOriginal_;
}

double Compressor::getAverageCompressionRatio() const {
    if (totalBytesOriginal_ == 0) {
        return 0.0;
    }
    return static_cast<double>(totalBytesCompressed_) / static_cast<double>(totalBytesOriginal_);
}

bool Compressor::compressFileInternal(FILE* source, FILE* dest, int level) {
    const size_t CHUNK = 16384;
    z_stream strm;
    uint8_t in[CHUNK];
    uint8_t out[CHUNK];
    int ret;
    
    // Initialize zlib
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    
    ret = deflateInit(&strm, level);
    if (ret != Z_OK) {
        std::cerr << "Error: Failed to initialize compression" << std::endl;
        return false;
    }
    
    // Compress until end of file
    int flush;
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            deflateEnd(&strm);
            std::cerr << "Error: Failed to read input file" << std::endl;
            return false;
        }
        
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;
        
        // Run deflate() on input until output buffer not full
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            
            ret = deflate(&strm, flush);
            if (ret == Z_STREAM_ERROR) {
                deflateEnd(&strm);
                std::cerr << "Error: Compression failed" << std::endl;
                return false;
            }
            
            size_t have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                deflateEnd(&strm);
                std::cerr << "Error: Failed to write compressed data" << std::endl;
                return false;
            }
        } while (strm.avail_out == 0);
        
        if (strm.avail_in != 0) {
            deflateEnd(&strm);
            std::cerr << "Error: Not all input consumed during compression" << std::endl;
            return false;
        }
        
    } while (flush != Z_FINISH);
    
    if (ret != Z_STREAM_END) {
        deflateEnd(&strm);
        std::cerr << "Error: Compression did not complete properly" << std::endl;
        return false;
    }
    
    deflateEnd(&strm);
    return true;
}

bool Compressor::decompressFileInternal(FILE* source, FILE* dest) {
    const size_t CHUNK = 16384;
    z_stream strm;
    uint8_t in[CHUNK];
    uint8_t out[CHUNK];
    int ret;
    
    // Initialize zlib
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    
    ret = inflateInit(&strm);
    if (ret != Z_OK) {
        std::cerr << "Error: Failed to initialize decompression" << std::endl;
        return false;
    }
    
    // Decompress until end of file
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            inflateEnd(&strm);
            std::cerr << "Error: Failed to read compressed file" << std::endl;
            return false;
        }
        
        if (strm.avail_in == 0) {
            break;
        }
        
        strm.next_in = in;
        
        // Run inflate() on input until output buffer not full
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            
            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT || 
                ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                inflateEnd(&strm);
                std::cerr << "Error: Decompression failed with error " << ret << std::endl;
                return false;
            }
            
            size_t have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                inflateEnd(&strm);
                std::cerr << "Error: Failed to write decompressed data" << std::endl;
                return false;
            }
        } while (strm.avail_out == 0);
        
    } while (ret != Z_STREAM_END);
    
    inflateEnd(&strm);
    
    if (ret != Z_STREAM_END) {
        std::cerr << "Error: Decompression did not complete properly" << std::endl;
        return false;
    }
    
    return true;
}

std::vector<uint8_t> Compressor::processData(const std::vector<uint8_t>& data, bool compress, int level) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    
    int ret;
    if (compress) {
        ret = deflateInit(&strm, level);
    } else {
        strm.avail_in = 0;
        strm.next_in = Z_NULL;
        ret = inflateInit(&strm);
    }
    
    if (ret != Z_OK) {
        return std::vector<uint8_t>();
    }
    
    strm.avail_in = data.size();
    strm.next_in = const_cast<uint8_t*>(data.data());
    
    std::vector<uint8_t> result;
    const size_t CHUNK = 16384;
    uint8_t out[CHUNK];
    
    do {
        strm.avail_out = CHUNK;
        strm.next_out = out;
        
        if (compress) {
            ret = deflate(&strm, Z_FINISH);
        } else {
            ret = inflate(&strm, Z_NO_FLUSH);
        }
        
        if (ret == Z_STREAM_ERROR || 
            (!compress && (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR))) {
            if (compress) {
                deflateEnd(&strm);
            } else {
                inflateEnd(&strm);
            }
            return std::vector<uint8_t>();
        }
        
        size_t have = CHUNK - strm.avail_out;
        result.insert(result.end(), out, out + have);
        
    } while (strm.avail_out == 0);
    
    if (compress) {
        deflateEnd(&strm);
        totalBytesOriginal_ += data.size();
        totalBytesCompressed_ += result.size();
    } else {
        inflateEnd(&strm);
    }
    
    return result;
}

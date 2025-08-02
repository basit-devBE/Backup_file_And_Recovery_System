#include "Encryptor.h"
#include "Utils.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

Encryptor::Encryptor() 
    : keySize_(KeySize::AES_256) {
    initializeEncryption();
}

Encryptor::~Encryptor() = default;

bool Encryptor::setKey(const std::string& key) {
    if (key.empty()) {
        return false;
    }
    
    // If key is hex-encoded, decode it
    if (key.length() == 64) { // 32 bytes * 2 hex chars
        key_.clear();
        for (size_t i = 0; i < key.length(); i += 2) {
            std::string byteStr = key.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
            key_.push_back(byte);
        }
    } else {
        // Use key as-is, pad or truncate to 32 bytes
        key_.assign(key.begin(), key.end());
        key_.resize(32, 0);
    }
    
    return true;
}

bool Encryptor::generateRandomKey(KeySize keySize) {
    keySize_ = keySize;
    size_t keyLength = static_cast<size_t>(keySize) / 8;
    
    key_ = generateRandomBytes(keyLength);
    return !key_.empty();
}

std::string Encryptor::getKeyHex() const {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (uint8_t byte : key_) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    
    return ss.str();
}

bool Encryptor::loadKeyFromFile(const std::string& keyFile) {
    std::ifstream file(keyFile, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    key_.clear();
    key_.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    
    return key_.size() == 32; // AES-256 key size
}

bool Encryptor::saveKeyToFile(const std::string& keyFile) {
    std::ofstream file(keyFile, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(key_.data()), key_.size());
    return file.good();
}

bool Encryptor::encryptFile(const std::string& inputFile, const std::string& outputFile) {
    if (key_.empty()) {
        std::cerr << "Error: No encryption key set" << std::endl;
        return false;
    }
    
    FILE* input = fopen(inputFile.c_str(), "rb");
    if (!input) {
        std::cerr << "Error: Cannot open input file: " << inputFile << std::endl;
        return false;
    }
    
    FILE* output = fopen(outputFile.c_str(), "wb");
    if (!output) {
        std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
        fclose(input);
        return false;
    }
    
    bool result = encryptFileInternal(input, output);
    
    fclose(input);
    fclose(output);
    
    return result;
}

bool Encryptor::decryptFile(const std::string& inputFile, const std::string& outputFile) {
    if (key_.empty()) {
        std::cerr << "Error: No decryption key set" << std::endl;
        return false;
    }
    
    FILE* input = fopen(inputFile.c_str(), "rb");
    if (!input) {
        std::cerr << "Error: Cannot open encrypted file: " << inputFile << std::endl;
        return false;
    }
    
    FILE* output = fopen(outputFile.c_str(), "wb");
    if (!output) {
        std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
        fclose(input);
        return false;
    }
    
    bool result = decryptFileInternal(input, output);
    
    fclose(input);
    fclose(output);
    
    return result;
}

std::vector<uint8_t> Encryptor::encryptData(const std::vector<uint8_t>& data) {
    return processData(data, true);
}

std::vector<uint8_t> Encryptor::decryptData(const std::vector<uint8_t>& encryptedData) {
    return processData(encryptedData, false);
}

std::string Encryptor::encryptString(const std::string& plaintext) {
    std::vector<uint8_t> data(plaintext.begin(), plaintext.end());
    std::vector<uint8_t> encrypted = encryptData(data);
    
    // Convert to hex string
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : encrypted) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    
    return ss.str();
}

std::string Encryptor::decryptString(const std::string& encrypted) {
    // Convert from hex string
    std::vector<uint8_t> data;
    for (size_t i = 0; i < encrypted.length(); i += 2) {
        std::string byteStr = encrypted.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
        data.push_back(byte);
    }
    
    std::vector<uint8_t> decrypted = decryptData(data);
    return std::string(decrypted.begin(), decrypted.end());
}

bool Encryptor::isEncrypted(const std::string& filePath) {
    // Simple heuristic: check if file starts with our custom header
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    char header[8];
    file.read(header, 8);
    
    if (file.gcount() < 8) {
        return false;
    }
    
    // Check for our encryption header
    return std::string(header, 8) == "ENCRYPT1";
}

std::string Encryptor::calculateHMAC(const std::string& data) {
    if (key_.empty()) {
        return "";
    }
    
    unsigned char* digest = HMAC(EVP_sha256(), 
                                key_.data(), key_.size(),
                                reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
                                nullptr, nullptr);
    
    if (!digest) {
        return "";
    }
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 32; i++) {
        ss << std::setw(2) << static_cast<int>(digest[i]);
    }
    
    return ss.str();
}

bool Encryptor::verifyHMAC(const std::string& data, const std::string& hmac) {
    std::string calculatedHMAC = calculateHMAC(data);
    return calculatedHMAC == hmac;
}

std::string Encryptor::deriveKeyFromPassword(const std::string& password, const std::string& salt) {
    const int iterations = 10000;
    const int keyLength = 32; // 256 bits
    
    std::vector<uint8_t> derivedKey(keyLength);
    
    int result = PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                                  reinterpret_cast<const unsigned char*>(salt.c_str()), salt.length(),
                                  iterations, EVP_sha256(),
                                  keyLength, derivedKey.data());
    
    if (result != 1) {
        return "";
    }
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : derivedKey) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    
    return ss.str();
}

std::string Encryptor::generateSalt() {
    auto saltBytes = generateRandomBytes(16);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : saltBytes) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    
    return ss.str();
}

bool Encryptor::initializeEncryption() {
    // Generate a random IV
    iv_ = generateRandomBytes(16); // AES block size
    return !iv_.empty();
}

std::vector<uint8_t> Encryptor::generateRandomBytes(size_t length) {
    std::vector<uint8_t> bytes(length);
    
    if (RAND_bytes(bytes.data(), length) != 1) {
        return std::vector<uint8_t>();
    }
    
    return bytes;
}

std::vector<uint8_t> Encryptor::processData(const std::vector<uint8_t>& data, bool encrypt) {
    if (key_.empty()) {
        return std::vector<uint8_t>();
    }
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return std::vector<uint8_t>();
    }
    
    std::vector<uint8_t> result;
    
    // Initialize cipher
    int ret;
    if (encrypt) {
        ret = EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key_.data(), iv_.data());
    } else {
        ret = EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key_.data(), iv_.data());
    }
    
    if (ret != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::vector<uint8_t>();
    }
    
    // Process data
    const size_t CHUNK_SIZE = 4096;
    std::vector<uint8_t> outBuffer(CHUNK_SIZE + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    
    for (size_t i = 0; i < data.size(); i += CHUNK_SIZE) {
        size_t chunkSize = std::min(CHUNK_SIZE, data.size() - i);
        int outLen;
        
        if (encrypt) {
            ret = EVP_EncryptUpdate(ctx, outBuffer.data(), &outLen, 
                                  data.data() + i, chunkSize);
        } else {
            ret = EVP_DecryptUpdate(ctx, outBuffer.data(), &outLen, 
                                  data.data() + i, chunkSize);
        }
        
        if (ret != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }
        
        result.insert(result.end(), outBuffer.begin(), outBuffer.begin() + outLen);
    }
    
    // Finalize
    int finalLen;
    if (encrypt) {
        ret = EVP_EncryptFinal_ex(ctx, outBuffer.data(), &finalLen);
    } else {
        ret = EVP_DecryptFinal_ex(ctx, outBuffer.data(), &finalLen);
    }
    
    if (ret == 1) {
        result.insert(result.end(), outBuffer.begin(), outBuffer.begin() + finalLen);
    }
    
    EVP_CIPHER_CTX_free(ctx);
    
    return (ret == 1) ? result : std::vector<uint8_t>();
}

bool Encryptor::encryptFileInternal(FILE* input, FILE* output) {
    // Write header and IV
    const char* header = "ENCRYPT1";
    if (fwrite(header, 1, 8, output) != 8) {
        return false;
    }
    
    if (fwrite(iv_.data(), 1, iv_.size(), output) != iv_.size()) {
        return false;
    }
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key_.data(), iv_.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    const size_t CHUNK_SIZE = 4096;
    std::vector<uint8_t> inBuffer(CHUNK_SIZE);
    std::vector<uint8_t> outBuffer(CHUNK_SIZE + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    
    size_t bytesRead;
    while ((bytesRead = fread(inBuffer.data(), 1, CHUNK_SIZE, input)) > 0) {
        int outLen;
        if (EVP_EncryptUpdate(ctx, outBuffer.data(), &outLen, inBuffer.data(), bytesRead) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        
        if (fwrite(outBuffer.data(), 1, outLen, output) != static_cast<size_t>(outLen)) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
    }
    
    int finalLen;
    if (EVP_EncryptFinal_ex(ctx, outBuffer.data(), &finalLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    if (fwrite(outBuffer.data(), 1, finalLen, output) != static_cast<size_t>(finalLen)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool Encryptor::decryptFileInternal(FILE* input, FILE* output) {
    // Read and verify header
    char header[8];
    if (fread(header, 1, 8, input) != 8 || std::string(header, 8) != "ENCRYPT1") {
        std::cerr << "Error: Invalid encryption header" << std::endl;
        return false;
    }
    
    // Read IV
    std::vector<uint8_t> fileIV(16);
    if (fread(fileIV.data(), 1, 16, input) != 16) {
        std::cerr << "Error: Cannot read IV from encrypted file" << std::endl;
        return false;
    }
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }
    
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key_.data(), fileIV.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    const size_t CHUNK_SIZE = 4096;
    std::vector<uint8_t> inBuffer(CHUNK_SIZE);
    std::vector<uint8_t> outBuffer(CHUNK_SIZE + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    
    size_t bytesRead;
    while ((bytesRead = fread(inBuffer.data(), 1, CHUNK_SIZE, input)) > 0) {
        int outLen;
        if (EVP_DecryptUpdate(ctx, outBuffer.data(), &outLen, inBuffer.data(), bytesRead) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        
        if (fwrite(outBuffer.data(), 1, outLen, output) != static_cast<size_t>(outLen)) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
    }
    
    int finalLen;
    if (EVP_DecryptFinal_ex(ctx, outBuffer.data(), &finalLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    if (fwrite(outBuffer.data(), 1, finalLen, output) != static_cast<size_t>(finalLen)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

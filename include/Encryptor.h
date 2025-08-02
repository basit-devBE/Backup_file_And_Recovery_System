#pragma once

#include <string>
#include <vector>
#include <cstdint>

/**
 * Handles file encryption and decryption using AES
 */
class Encryptor {
public:
    enum class KeySize {
        AES_128 = 128,
        AES_192 = 192,
        AES_256 = 256
    };

    Encryptor();
    ~Encryptor();

    // Key management
    bool setKey(const std::string& key);
    bool generateRandomKey(KeySize keySize = KeySize::AES_256);
    std::string getKeyHex() const;
    bool loadKeyFromFile(const std::string& keyFile);
    bool saveKeyToFile(const std::string& keyFile);
    
    // File encryption
    bool encryptFile(const std::string& inputFile, const std::string& outputFile);
    bool decryptFile(const std::string& inputFile, const std::string& outputFile);
    
    // Data encryption
    std::vector<uint8_t> encryptData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decryptData(const std::vector<uint8_t>& encryptedData);
    
    // String encryption
    std::string encryptString(const std::string& plaintext);
    std::string decryptString(const std::string& encrypted);
    
    // Utility functions
    bool isEncrypted(const std::string& filePath);
    std::string calculateHMAC(const std::string& data);
    bool verifyHMAC(const std::string& data, const std::string& hmac);
    
    // Key derivation
    std::string deriveKeyFromPassword(const std::string& password, const std::string& salt);
    std::string generateSalt();

private:
    std::vector<uint8_t> key_;
    std::vector<uint8_t> iv_;
    KeySize keySize_;
    
    // Helper methods
    bool initializeEncryption();
    std::vector<uint8_t> generateRandomBytes(size_t length);
    std::vector<uint8_t> processData(const std::vector<uint8_t>& data, bool encrypt);
    bool encryptFileInternal(FILE* input, FILE* output);
    bool decryptFileInternal(FILE* input, FILE* output);
};

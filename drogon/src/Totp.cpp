#include "Totp.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <random>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <ctime>

std::vector<unsigned char> Totp::base32Decode(const std::string& base32Str) {
    std::vector<unsigned char> decoded;
    int buffer = 0;
    int bitsLeft = 0;
    
    for (char c : base32Str) {
        int val = 0;
        if (c >= 'A' && c <= 'Z') val = c - 'A';
        else if (c >= 'a' && c <= 'z') val = c - 'a';
        else if (c >= '2' && c <= '7') val = c - '2' + 26;
        else continue; // ignore padding or invalid chars
        
        buffer = (buffer << 5) | val;
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            decoded.push_back((unsigned char)((buffer >> (bitsLeft - 8)) & 0xFF));
            bitsLeft -= 8;
        }
    }
    return decoded;
}

std::string Totp::generateSecret(int length) {
    const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    std::string secret;
    secret.reserve(length);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 31);
    
    for (int i = 0; i < length; ++i) {
        secret += chars[dis(gen)];
    }
    return secret;
}

std::string Totp::generateCode(const std::string& secretBase32, uint64_t timeStep) {
    std::vector<unsigned char> key = base32Decode(secretBase32);
    if (key.empty()) return "000000";

    // 8 bytes of time_step in big-endian
    unsigned char msg[8];
    for (int i = 7; i >= 0; --i) {
        msg[i] = timeStep & 0xFF;
        timeStep >>= 8;
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    // OpenSSL 3.0+ HMAC is deprecated, but we can still use it for now, or use EVP_MAC
    // To avoid deprecated warnings on OpenSSL 3, we'd use EVP_MAC, but HMAC() is shorter.
#endif
    HMAC(EVP_sha1(), key.data(), key.size(), msg, 8, hash, &hashLen);

    // Dynamic truncation
    int offset = hash[19] & 0x0F;
    uint32_t binary =
        ((hash[offset] & 0x7F) << 24) |
        ((hash[offset + 1] & 0xFF) << 16) |
        ((hash[offset + 2] & 0xFF) << 8) |
        (hash[offset + 3] & 0xFF);

    uint32_t otp = binary % 1000000;
    
    std::ostringstream oss;
    oss << std::setw(6) << std::setfill('0') << otp;
    return oss.str();
}

bool Totp::verify(const std::string& secretBase32, const std::string& code) {
    uint64_t currentStep = std::time(nullptr) / 30;
    
    // Check current, previous, and next step to account for clock drift
    for (int i = -1; i <= 1; ++i) {
        if (generateCode(secretBase32, currentStep + i) == code) {
            return true;
        }
    }
    return false;
}

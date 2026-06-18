#pragma once

#include <string>
#include <vector>

class Totp {
public:
    // Generate a random Base32 secret string of specified length (default 16 chars = 80 bits)
    static std::string generateSecret(int length = 16);

    // Verify a 6-digit TOTP code against the secret. Allows for +/- 1 time step (30 seconds each)
    static bool verify(const std::string& secretBase32, const std::string& code);

    // Generate the 6-digit TOTP code for a specific time step
    static std::string generateCode(const std::string& secretBase32, uint64_t timeStep);

private:
    static std::vector<unsigned char> base32Decode(const std::string& base32Str);
};

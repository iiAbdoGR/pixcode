#include "Argon2idPasswordHasher.h"
#include <sodium.h>
#include <stdexcept>
#include <cctype>

namespace pixcode::infrastructure {

Argon2idPasswordHasher::Argon2idPasswordHasher() {
    if (sodium_init() < 0) {
        throw std::runtime_error("Failed to initialize libsodium for Argon2id hashing");
    }
}

std::string Argon2idPasswordHasher::hashPassword(std::string_view plaintext) {
    char outHash[crypto_pwhash_STRBYTES];

    // Using crypto_pwhash_OPSLIMIT_INTERACTIVE and crypto_pwhash_MEMLIMIT_INTERACTIVE
    // for optimal desktop responsiveness while maintaining strong side-channel and GPU resistance
    if (crypto_pwhash_str(outHash,
                          plaintext.data(),
                          plaintext.size(),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        throw std::runtime_error("Argon2id password hashing failed (out of memory or invalid parameters)");
    }

    return std::string(outHash);
}

bool Argon2idPasswordHasher::verifyPassword(std::string_view plaintext, std::string_view hash) {
    if (hash.empty()) return false;

    return crypto_pwhash_str_verify(hash.data(),
                                    plaintext.data(),
                                    plaintext.size()) == 0;
}

bool Argon2idPasswordHasher::isPasswordStrong(std::string_view plaintext, std::string& outFailureReason) const {
    if (plaintext.length() < 8) {
        outFailureReason = "Password must be at least 8 characters long";
        return false;
    }

    bool hasUpper = false;
    bool hasLower = false;
    bool hasDigit = false;

    for (const char c : plaintext) {
        if (std::isupper(static_cast<unsigned char>(c))) hasUpper = true;
        if (std::islower(static_cast<unsigned char>(c))) hasLower = true;
        if (std::isdigit(static_cast<unsigned char>(c))) hasDigit = true;
    }

    if (!hasUpper || !hasLower || !hasDigit) {
        outFailureReason = "Password must contain uppercase letters, lowercase letters, and numbers";
        return false;
    }

    return true;
}

} // namespace pixcode::infrastructure


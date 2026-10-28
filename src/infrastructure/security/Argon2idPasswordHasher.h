#pragma once

#include "application/ports/IPasswordHasher.h"

namespace pixcode::infrastructure {

class Argon2idPasswordHasher : public application::IPasswordHasher {
public:
    Argon2idPasswordHasher();

    [[nodiscard]] std::string hashPassword(std::string_view plaintext) override;
    [[nodiscard]] bool verifyPassword(std::string_view plaintext, std::string_view hash) override;
    [[nodiscard]] bool isPasswordStrong(std::string_view plaintext, std::string& outFailureReason) const override;
};

} // namespace pixcode::infrastructure


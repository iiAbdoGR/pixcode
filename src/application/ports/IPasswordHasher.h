#pragma once

#include <string>
#include <string_view>

namespace pixcode::application {

class IPasswordHasher {
public:
    virtual ~IPasswordHasher() = default;

    [[nodiscard]] virtual std::string hashPassword(std::string_view plaintext) = 0;
    [[nodiscard]] virtual bool verifyPassword(std::string_view plaintext, std::string_view hash) = 0;
    [[nodiscard]] virtual bool isPasswordStrong(std::string_view plaintext, std::string& outFailureReason) const = 0;
};

} // namespace pixcode::application


#pragma once

#include <string>

namespace pixcode::domain {

class Permission {
public:
    Permission(std::string id, std::string code, std::string displayKey, std::string descriptionKey)
        : m_id(std::move(id)), m_code(std::move(code)), m_displayKey(std::move(displayKey)), m_descriptionKey(std::move(descriptionKey)) {}

    [[nodiscard]] const std::string& id() const noexcept { return m_id; }
    [[nodiscard]] const std::string& code() const noexcept { return m_code; }
    [[nodiscard]] const std::string& displayKey() const noexcept { return m_displayKey; }
    [[nodiscard]] const std::string& descriptionKey() const noexcept { return m_descriptionKey; }

private:
    std::string m_id;
    std::string m_code;
    std::string m_displayKey;
    std::string m_descriptionKey;
};

} // namespace pixcode::domain


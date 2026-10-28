#include "Money.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace pixcode::domain {

std::string Money::format(std::string_view currencyCode) const {
    std::ostringstream oss;
    if (m_piasters < 0) {
        oss << "-";
    }
    const auto absPiasters = std::abs(m_piasters);
    oss << (absPiasters / 100) << "."
        << std::setw(2) << std::setfill('0') << (absPiasters % 100)
        << " " << currencyCode;
    return oss.str();
}

std::vector<Money> Money::splitEqually(std::size_t parts) const {
    if (parts == 0) {
        throw std::invalid_argument("Cannot split money into 0 parts");
    }

    const auto basePart = m_piasters / static_cast<std::int64_t>(parts);
    const auto remainder = m_piasters % static_cast<std::int64_t>(parts);

    std::vector<Money> result;
    result.reserve(parts);

    // Rule: remainder goes to the first part
    for (std::size_t i = 0; i < parts; ++i) {
        if (i == 0) {
            result.emplace_back(basePart + remainder);
        } else {
            result.emplace_back(basePart);
        }
    }

    return result;
}

} // namespace pixcode::domain


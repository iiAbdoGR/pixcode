#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pixcode::domain {

class Money {
public:
    constexpr Money() noexcept : m_piasters(0) {}
    constexpr explicit Money(std::int64_t piasters) noexcept : m_piasters(piasters) {}

    [[nodiscard]] static constexpr Money fromEgp(std::int64_t egp) noexcept {
        return Money(egp * 100);
    }

    [[nodiscard]] static Money fromPiasters(std::int64_t piasters) noexcept {
        return Money(piasters);
    }

    [[nodiscard]] constexpr std::int64_t piasters() const noexcept { return m_piasters; }
    [[nodiscard]] constexpr std::int64_t wholeEgp() const noexcept { return m_piasters / 100; }
    [[nodiscard]] constexpr std::int64_t fractionalPiasters() const noexcept {
        const auto rem = m_piasters % 100;
        return rem < 0 ? -rem : rem;
    }

    [[nodiscard]] std::string format(std::string_view currencyCode = "EGP") const;

    // Installment remainder allocation: split amount into N parts, assign remainder to first part
    [[nodiscard]] std::vector<Money> splitEqually(std::size_t parts) const;

    constexpr Money operator+(const Money& other) const noexcept {
        return Money(m_piasters + other.m_piasters);
    }

    constexpr Money operator-(const Money& other) const noexcept {
        return Money(m_piasters - other.m_piasters);
    }

    constexpr Money operator*(std::int64_t factor) const noexcept {
        return Money(m_piasters * factor);
    }

    constexpr bool operator==(const Money& other) const noexcept = default;
    constexpr auto operator<=>(const Money& other) const noexcept = default;

private:
    std::int64_t m_piasters;
};

} // namespace pixcode::domain


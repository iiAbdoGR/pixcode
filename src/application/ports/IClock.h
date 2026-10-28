#pragma once

#include <string>

namespace pixcode::application {

class IClock {
public:
    virtual ~IClock() = default;
    [[nodiscard]] virtual std::string nowUtcIso() const = 0;
    [[nodiscard]] virtual std::string addMinutesUtcIso(int minutes) const = 0;
};

} // namespace pixcode::application


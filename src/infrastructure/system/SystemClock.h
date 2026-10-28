#pragma once

#include "application/ports/IClock.h"

namespace pixcode::infrastructure {

class SystemClock : public application::IClock {
public:
    [[nodiscard]] std::string nowUtcIso() const override;
    [[nodiscard]] std::string addMinutesUtcIso(int minutes) const override;
};

} // namespace pixcode::infrastructure


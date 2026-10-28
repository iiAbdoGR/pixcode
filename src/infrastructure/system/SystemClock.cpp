#include "SystemClock.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace pixcode::infrastructure {

static std::string formatTimePointUtc(std::chrono::system_clock::time_point tp) {
    const auto time = std::chrono::system_clock::to_time_t(tp);
    std::tm tmUtc{};
#if defined(_WIN32)
    gmtime_s(&tmUtc, &time);
#else
    gmtime_r(&time, &tmUtc);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmUtc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string SystemClock::nowUtcIso() const {
    return formatTimePointUtc(std::chrono::system_clock::now());
}

std::string SystemClock::addMinutesUtcIso(int minutes) const {
    const auto future = std::chrono::system_clock::now() + std::chrono::minutes(minutes);
    return formatTimePointUtc(future);
}

} // namespace pixcode::infrastructure


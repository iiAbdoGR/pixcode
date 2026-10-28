#pragma once

#include <QString>

namespace pixcode::desktop {

class Theme {
public:
    static QString getApplicationStyleSheet();

    // Color constants
    static constexpr const char* Primary = "#2E3A44";
    static constexpr const char* Secondary = "#4C5F6B";
    static constexpr const char* Slate = "#7E939C";
    static constexpr const char* SoftBlue = "#B0C2C6";
    static constexpr const char* Sage = "#DDE4E1";
    static constexpr const char* Background = "#F6F4EE";
    static constexpr const char* Surface = "#FFFFFF";
    static constexpr const char* TextPrimary = "#1F2933";
    static constexpr const char* TextSecondary = "#52606D";
    static constexpr const char* Border = "#CBD2D9";
    static constexpr const char* Danger = "#D32F2F";
    static constexpr const char* Success = "#2E7D32";
    static constexpr const char* Warning = "#ED6C02";
};

} // namespace pixcode::desktop


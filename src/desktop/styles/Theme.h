#pragma once

#include <QString>
#include <QtWidgets/QApplication>
#include <QtGui/QPalette>
#include <QtGui/QColor>

namespace pixcode::desktop {

class Theme {
public:
    static QString getApplicationStyleSheet();
    static void setupApplicationPalette(QApplication& app);

    // Authentic Pix Code Academy 7-color Blue Palette
    static constexpr const char* BrandNavy = "#001D39";
    static constexpr const char* Primary = "#0A4174";
    static constexpr const char* Secondary = "#49769F";
    static constexpr const char* OceanBlue = "#4E8EA2";
    static constexpr const char* SlateBlue = "#6EA2B3";
    static constexpr const char* SkyBlue = "#7BBDE8";
    static constexpr const char* IceBlue = "#BDD8E9";

    static constexpr const char* Background = "#F8FAFC";
    static constexpr const char* Surface = "#FFFFFF";
    static constexpr const char* TextPrimary = "#001D39";
    static constexpr const char* TextSecondary = "#49769F";
    static constexpr const char* Border = "#BDD8E9";
    static constexpr const char* Danger = "#D32F2F";
    static constexpr const char* Success = "#2E7D32";
    static constexpr const char* Warning = "#ED6C02";

    // Palette compatibility aliases
    static constexpr const char* Slate = SlateBlue;
    static constexpr const char* SoftBlue = SkyBlue;
    static constexpr const char* Sage = IceBlue;
    static constexpr const char* BackgroundLight = Background;
    static constexpr const char* PrimaryDark = BrandNavy;
    static constexpr const char* SecondaryDark = Primary;
    static constexpr const char* SurfaceWhite = Surface;
    static constexpr const char* SurfaceLight = IceBlue;
    static constexpr const char* SurfaceBorder = Border;
};

} // namespace pixcode::desktop

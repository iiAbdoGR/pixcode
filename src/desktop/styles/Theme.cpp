#include "Theme.h"

namespace pixcode::desktop {

void Theme::setupApplicationPalette(QApplication& app) {
    QPalette p;
    p.setColor(QPalette::Window, QColor(Background));
    p.setColor(QPalette::WindowText, QColor(TextPrimary));
    p.setColor(QPalette::Base, QColor(Surface));
    p.setColor(QPalette::AlternateBase, QColor(IceBlue));
    p.setColor(QPalette::ToolTipBase, QColor(BrandNavy));
    p.setColor(QPalette::ToolTipText, QColor("#FFFFFF"));
    p.setColor(QPalette::Text, QColor(TextPrimary));
    p.setColor(QPalette::Button, QColor(Primary));
    p.setColor(QPalette::ButtonText, QColor("#FFFFFF"));
    p.setColor(QPalette::BrightText, QColor(Danger));
    p.setColor(QPalette::Link, QColor(Primary));
    p.setColor(QPalette::Highlight, QColor(Primary));
    p.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    app.setPalette(p);
}

QString Theme::getApplicationStyleSheet() {
    return QString(R"(
        QWidget {
            color: %2;
            font-family: "Segoe UI", "Cairo", "Tajawal", "Arial", sans-serif;
            font-size: 13px;
        }

        QMainWindow, QDialog, QStackedWidget {
            background-color: %1;
        }

        /* Input Controls */
        QLineEdit, QTextEdit, QComboBox, QSpinBox, QDateEdit, QTimeEdit {
            background-color: %3;
            color: %2;
            border: 1px solid %4;
            border-radius: 6px;
            padding: 8px 12px;
            selection-background-color: %5;
            selection-color: %3;
        }

        QLineEdit:focus, QTextEdit:focus, QComboBox:focus, QSpinBox:focus, QDateEdit:focus, QTimeEdit:focus {
            border: 2px solid %6;
            padding: 7px 11px;
        }

        /* Buttons */
        QPushButton {
            background-color: %6;
            color: #FFFFFF;
            border: none;
            border-radius: 6px;
            padding: 8px 18px;
            font-weight: 600;
        }

        QPushButton:hover {
            background-color: %7;
        }

        QPushButton:pressed {
            background-color: %8;
        }

        QPushButton:disabled {
            background-color: %4;
            color: %9;
        }

        QPushButton#secondaryButton {
            background-color: transparent;
            color: %6;
            border: 1px solid %6;
        }

        QPushButton#secondaryButton:hover {
            background-color: %10;
        }

        QPushButton#dangerButton {
            background-color: %11;
            color: #FFFFFF;
        }

        QPushButton#dangerButton:hover {
            background-color: #B71C1C;
        }

        /* Table & Lists */
        QTableWidget, QTableView {
            background-color: %3;
            color: %2;
            gridline-color: %12;
            border: 1px solid %4;
            border-radius: 6px;
            selection-background-color: %6;
            selection-color: #FFFFFF;
        }

        QTableWidget::item, QTableView::item {
            padding: 6px 10px;
            border: none;
        }

        QTableWidget::item:hover, QTableView::item:hover {
            background-color: %10;
        }

        QTableWidget::item:selected, QTableView::item:selected {
            background-color: %6;
            color: #FFFFFF;
            font-weight: 600;
        }

        QTableWidget::item:selected:focus, QTableView::item:selected:focus {
            background-color: %8;
            color: #FFFFFF;
        }

        QTableWidget:!focus QTableWidget::item:selected, QTableView:!focus QTableView::item:selected {
            background-color: %7;
            color: #FFFFFF;
        }

        QHeaderView::section {
            background-color: %6;
            color: #FFFFFF;
            padding: 8px 12px;
            font-weight: 600;
            border: none;
            border-right: 1px solid %7;
        }

        /* Navigation Sidebar */
        QFrame#navigationSidebar {
            background-color: %8;
            border-right: 1px solid %7;
        }

        QPushButton#navButton {
            background-color: transparent;
            color: %9;
            text-align: left;
            padding: 12px 18px;
            border-radius: 0px;
            font-size: 14px;
            font-weight: 500;
        }

        QPushButton#navButton:hover {
            background-color: %7;
            color: #FFFFFF;
        }

        QPushButton#navButton:checked {
            background-color: %6;
            color: #FFFFFF;
            font-weight: 700;
            border-left: 4px solid %9;
        }

        /* Status / Badges */
        QLabel#headerTitle {
            font-size: 20px;
            font-weight: 700;
            color: %8;
        }

        QLabel#errorLabel {
            color: %11;
            font-weight: 600;
        }

        QLabel#successLabel {
            color: %13;
            font-weight: 600;
        }
    )")
    .arg(Background)      // %1
    .arg(TextPrimary)     // %2
    .arg(Surface)         // %3
    .arg(Border)          // %4
    .arg(Secondary)       // %5
    .arg(Primary)         // %6
    .arg(Secondary)       // %7
    .arg(BrandNavy)       // %8
    .arg(SkyBlue)         // %9
    .arg(IceBlue)         // %10
    .arg(Danger)          // %11
    .arg(Border)          // %12
    .arg(Success);        // %13
}

} // namespace pixcode::desktop

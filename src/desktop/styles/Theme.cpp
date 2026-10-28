#include "Theme.h"

namespace pixcode::desktop {

QString Theme::getApplicationStyleSheet() {
    return QString(R"(
        QWidget {
            background-color: %1;
            color: %2;
            font-family: "Segoe UI", "Cairo", "Tajawal", "Arial", sans-serif;
            font-size: 13px;
        }

        QMainWindow, QDialog {
            background-color: %1;
        }

        /* Input Controls */
        QLineEdit, QTextEdit, QComboBox, QSpinBox {
            background-color: %3;
            color: %2;
            border: 1px solid %4;
            border-radius: 6px;
            padding: 8px 12px;
            selection-background-color: %5;
            selection-color: %3;
        }

        QLineEdit:focus, QTextEdit:focus, QComboBox:focus, QSpinBox:focus {
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
            background-color: %9;
            color: #8D99AE;
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
            selection-background-color: %10;
            selection-color: %2;
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
    .arg(Secondary)       // %6
    .arg(Slate)           // %7
    .arg(Primary)         // %8
    .arg(SoftBlue)        // %9
    .arg(Sage)            // %10
    .arg(Danger)          // %11
    .arg(Border)          // %12
    .arg(Success);        // %13
}

} // namespace pixcode::desktop


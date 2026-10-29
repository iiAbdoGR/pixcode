#include <QtWidgets/QApplication>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtWidgets/QMessageBox>
#include "app/ApplicationContext.h"
#include "styles/Theme.h"
#include "localization/LocalizationManager.h"
#include "dialogs/FirstRunWizardDialog.h"
#include "dialogs/LoginDialog.h"
#include "views/MainWindow.h"
#include "infrastructure/logging/AppLogger.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("PixCodeAcademy");
    app.setOrganizationName("PixCode");
    pixcode::desktop::LocalizationManager::instance().setLanguage("ar");
    app.setLayoutDirection(Qt::RightToLeft);
    pixcode::desktop::Theme::setupApplicationPalette(app);
    app.setStyleSheet(pixcode::desktop::Theme::getApplicationStyleSheet());

    // Resolve app data directory
    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataDir);

    // Initialize Rotating Logger
    pixcode::infrastructure::AppLogger::init(appDataDir + "/logs");

    const QString dbPath = appDataDir + "/pixcode.db";
    auto appContext = std::make_shared<pixcode::desktop::ApplicationContext>(dbPath);

    QString error;
    if (!appContext->initialize(error)) {
        QMessageBox::critical(nullptr, "Startup Failure",
                              "Failed to initialize database and migrations: " + error);
        return 1;
    }

    // Check if initial setup is required
    if (appContext->firstRunUseCase()->isFirstRunRequired()) {
        pixcode::desktop::FirstRunWizardDialog wizard(appContext);
        if (wizard.exec() != QDialog::Accepted) {
            // User aborted initial setup
            return 0;
        }
    }

    // Launch Login Dialog
    pixcode::desktop::LoginDialog loginDialog(appContext);
    if (loginDialog.exec() != QDialog::Accepted) {
        return 0;
    }

    pixcode::desktop::MainWindow mainWindow(appContext);
    mainWindow.show();

    const int exitCode = app.exec();
    pixcode::infrastructure::AppLogger::shutdown();
    return exitCode;
}
